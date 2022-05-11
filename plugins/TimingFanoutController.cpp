/**
 * @file TimingFanoutController.cpp TimingFanoutController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingFanoutController.hpp"
#include "timinglibs/timingfanoutcontroller/Nljs.hpp"
#include "timinglibs/timingfanoutcontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timinghardwareinfo/InfoNljs.hpp"
#include "timing/timinghardwareinfo/InfoStructs.hpp"

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

TimingFanoutController::TimingFanoutController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 2) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingFanoutController::do_configure);
  register_command("start", &TimingFanoutController::do_start);
  register_command("stop", &TimingFanoutController::do_stop);
  register_command("scrap", &TimingFanoutController::do_scrap);
  
  // timing fanout hardware commands
  register_command("fanout_io_reset", &TimingFanoutController::do_fanout_io_reset);
  register_command("fanout_print_status", &TimingFanoutController::do_fanout_print_status);
}

void
TimingFanoutController::do_configure(const nlohmann::json& data)
{
  auto conf = data.get<timingfanoutcontroller::ConfParams>();
  if (conf.device.empty())
  {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name should not be empty");
  }
  m_timing_device = conf.device;

  TimingController::do_configure(data); // configure hw command connection

  do_fanout_io_reset(data);
  
  auto time_of_conf = std::chrono::high_resolution_clock::now();
  while (!m_device_ready)
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms_since_conf = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_of_conf);
    
    if (ms_since_conf > m_device_ready_timeout)
    {
      throw TimingFanoutNotReady(ERS_HERE,m_timing_device);
    }
    TLOG_DEBUG(3) << "Waiting for timing fanout " << m_timing_device << " to become ready for (ms) " << ms_since_conf.count();
    std::this_thread::sleep_for(std::chrono::microseconds(250000));
  }

  TLOG() << get_name() << "conf: fanout device: " << m_timing_device;
}

timingcmd::TimingHwCmd
TimingFanoutController::construct_fanout_hw_cmd( const std::string& cmd_id)
{
    timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
TimingFanoutController::do_fanout_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd = 
  construct_fanout_hw_cmd( "io_reset");
  hw_cmd.payload = data;
  hw_cmd.payload["fanout_mode"] = 0; // fanout mode for fanout design

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingFanoutController::do_fanout_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_fanout_hw_cmd( "print_status");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingFanoutController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  timingfanoutcontrollerinfo::Info module_info;

  module_info.sent_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_print_status_cmds = m_sent_hw_command_counters.at(1).atomic.load();

  ci.add(module_info);
}

void
TimingFanoutController::process_device_info(nlohmann::json info)
{
  timing::timinghardwareinfo::TimingPC059MonitorData pc059_info;

  auto io_data = info[opmonlib::JSONTags::children]["io"][opmonlib::JSONTags::properties][pc059_info.info_type][opmonlib::JSONTags::data];

  from_json(io_data, pc059_info);

  bool ucdr_los = pc059_info.ucdr_lol;
  bool ucdr_lol = pc059_info.ucdr_los;

  TLOG_DEBUG(3) << "ucdr_los: " << ucdr_los << ", ucdr_lol: " << ucdr_lol;

  if (!ucdr_lol && !ucdr_los)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "Timing fanout became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "Timing fanout no longer ready";
    }
  }
  ++m_device_infos_received_count;
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingFanoutController)

// Local Variables:
// c-basic-offset: 2
// End:
