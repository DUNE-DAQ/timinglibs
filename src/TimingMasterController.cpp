/**
 * @file TimingMasterController.cpp TimingMasterController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingMasterController.hpp"
#include "timinglibs/timingmastercontroller/Nljs.hpp"
#include "timinglibs/timingmastercontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingMasterController::TimingMasterController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 7) // 2nd arg: how many hw commands can this module send?
  , m_endpoint_scan_period(0)
  , endpoint_scan_thread(std::bind(&TimingMasterController::endpoint_scan, this, std::placeholders::_1))
{
  register_command("conf", &TimingMasterController::do_configure);
  register_command("start", &TimingMasterController::do_start);
  register_command("stop", &TimingMasterController::do_stop);
  register_command("scrap", &TimingMasterController::do_scrap);

  // timing master hardware commands
  register_command("master_io_reset", &TimingMasterController::do_master_io_reset);
  register_command("master_set_timestamp", &TimingMasterController::do_master_set_timestamp);
  register_command("master_print_status", &TimingMasterController::do_master_print_status);
  register_command("master_set_endpoint_delay", &TimingMasterController::do_master_set_endpoint_delay);
  register_command("master_send_fl_command", &TimingMasterController::do_master_send_fl_command);
  register_command("master_measure_endpoint_rtt", &TimingMasterController::do_master_measure_endpoint_rtt);
  register_command("master_endpoint_scan", &TimingMasterController::do_master_endpoint_scan);
}

void
TimingMasterController::do_configure(const nlohmann::json& data)
{
  auto conf = data.get<timingmastercontroller::ConfParams>();
  if (conf.device.empty())
  {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name should not be empty");
  }
  m_timing_device = conf.device;
  m_hardware_state_recovery_enabled = conf.hardware_state_recovery_enabled;
  m_timing_session_name = conf.timing_session_name;
  m_monitored_endpoint_locations = conf.monitored_endpoints;

  TimingController::do_configure(data); // configure hw command connection

  configure_hardware_or_recover_state<TimingMasterNotReady>(data, "Timing master");

  TLOG() << get_name() << " conf done on master, device: " << m_timing_device;

  m_endpoint_scan_period = conf.endpoint_scan_period;
  if (m_endpoint_scan_period)
  {
    TLOG() << get_name() << " conf: master, will send delays with period [ms] " << m_endpoint_scan_period;    
  }
  else
  {
    TLOG() << get_name() << " conf: master, will not send delays";
  }
}

void
TimingMasterController::do_start(const nlohmann::json& data)
{
  TimingController::do_start(data); // set sent cmd counters to 0
  if (m_endpoint_scan_period) endpoint_scan_thread.start_working_thread();
}

void
TimingMasterController::do_stop(const nlohmann::json& /*data*/)
{
  if (endpoint_scan_thread.thread_running()) endpoint_scan_thread.stop_working_thread();
}

void
TimingMasterController::send_configure_hardware_commands(const nlohmann::json& data)
{
  do_master_io_reset(data);
  do_master_set_timestamp(data);
}

timingcmd::TimingHwCmd
TimingMasterController::construct_master_hw_cmd(const std::string& cmd_id)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
TimingMasterController::do_master_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "io_reset");
  hw_cmd.payload = data;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingMasterController::do_master_set_timestamp(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "set_timestamp");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingMasterController::do_master_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "print_status");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingMasterController::do_master_set_endpoint_delay(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "set_endpoint_delay");
  hw_cmd.payload = data;
  
  TLOG_DEBUG(2) << "set ept delay data: " << data.dump();
  
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingMasterController::do_master_send_fl_command(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "send_fl_command");
  hw_cmd.payload = data;
  
  TLOG_DEBUG(2) << "send fl cmd data: " << data.dump();

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
TimingMasterController::do_master_measure_endpoint_rtt(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "master_measure_endpoint_rtt");
  hw_cmd.payload = data;
  
  TLOG_DEBUG(2) << "measure endpoint rtt data: " << data.dump();

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
TimingMasterController::do_master_endpoint_scan(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_master_hw_cmd( "master_endpoint_scan");
  hw_cmd.payload = data;
  
  TLOG_DEBUG(2) << "endpoint scan data: " << data.dump();

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
TimingMasterController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  timingmastercontrollerinfo::Info module_info;
  module_info.sent_master_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_master_set_timestamp_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_master_print_status_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  module_info.sent_master_set_endpoint_delay_cmds = m_sent_hw_command_counters.at(3).atomic.load();
  module_info.sent_master_send_fl_command_cmds = m_sent_hw_command_counters.at(4).atomic.load();

  // for (uint i = 0; i < m_number_hw_commands; ++i) {
  //  module_info.sent_hw_command_counters.push_back(m_sent_hw_command_counters.at(i).atomic.load());
  //}
  ci.add(module_info);
}

// cmd stuff
void
TimingMasterController::endpoint_scan(std::atomic<bool>& running_flag)
{

  std::ostringstream starting_stream;
  starting_stream << ": Starting endpoint_scan() method.";
  TLOG_DEBUG(0) << get_name() << starting_stream.str();

  while (running_flag.load() && m_endpoint_scan_period) {

    timingcmd::TimingHwCmd hw_cmd =
    construct_master_hw_cmd( "master_endpoint_scan");

    timingcmd::TimingMasterEndpointScanPayload cmd_payload;
    cmd_payload.endpoints = m_monitored_endpoint_locations;
    
    hw_cmd.payload = cmd_payload;
    send_hw_cmd(std::move(hw_cmd));

    ++(m_sent_hw_command_counters.at(3).atomic);
    if (m_endpoint_scan_period)
    {
      auto prev_gather_time = std::chrono::steady_clock::now();
      auto next_gather_time = prev_gather_time + std::chrono::milliseconds(m_endpoint_scan_period);

      // check running_flag periodically
      auto slice_period = std::chrono::microseconds(10000);
      auto next_slice_gather_time = prev_gather_time + slice_period;

      bool break_flag = false;
      while (next_gather_time > next_slice_gather_time + slice_period) {
        if (!running_flag.load()) {
          TLOG_DEBUG(0) << "while waiting to send delays, negative run gatherer flag detected.";
          break_flag = true;
          break;
        }
        std::this_thread::sleep_until(next_slice_gather_time);
        next_slice_gather_time = next_slice_gather_time + slice_period;
      }
      if (break_flag == false) {
        std::this_thread::sleep_until(next_gather_time);
      }
    }
    else
    {
      TLOG() << "m_endpoint_scan_period is 0 and send delays thread is running! breaking loop!";
      break;
    }
  }

  std::ostringstream exiting_stream;
  exiting_stream << ": Exiting endpoint_scan() method. Received " <<  m_sent_hw_command_counters.at(3).atomic.load()
                 << " commands";
  TLOG_DEBUG(0) << get_name() << exiting_stream.str();
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
