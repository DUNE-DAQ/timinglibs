/**
 * @file HSIController.cpp HSIController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIController.hpp"
#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/hsicontroller/Nljs.hpp"
#include "timinglibs/hsicontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"

#include "opmonlib/JSONTags.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

HSIController::HSIController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 9) // 2nd arg: how many hw commands can this module send?
  , m_clock_frequency(50e6)
{
  register_command("conf", &HSIController::do_configure);
  register_command("start", &HSIController::do_start);
  register_command("stop_trigger_sources", &HSIController::do_stop);
  register_command("change_rate", &HSIController::do_change_rate);
  register_command("scrap", &HSIController::do_scrap);
  
  // timing endpoint hardware commands
  register_command("hsi_io_reset", &HSIController::do_hsi_io_reset);
  register_command("hsi_endpoint_enable", &HSIController::do_hsi_endpoint_enable);
  register_command("hsi_endpoint_disable", &HSIController::do_hsi_endpoint_disable);
  register_command("hsi_endpoint_reset", &HSIController::do_hsi_endpoint_reset);
  register_command("hsi_reset", &HSIController::do_hsi_reset);
  register_command("hsi_configure", &HSIController::do_hsi_configure);
  register_command("hsi_start", &HSIController::do_hsi_start);
  register_command("hsi_stop", &HSIController::do_hsi_stop);
  register_command("hsi_print_status", &HSIController::do_hsi_print_status);
}

void
HSIController::do_configure(const nlohmann::json& data)
{
  m_hsi_configuration = data.get<hsicontroller::ConfParams>();
  m_timing_device = m_hsi_configuration.device;
  
  TimingController::do_configure(data); // configure hw command connection

  do_hsi_reset(data);
  do_hsi_endpoint_reset(data);
  do_hsi_configure(data);

  auto time_of_conf = std::chrono::high_resolution_clock::now();
  while (!m_device_ready)
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms_since_conf = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_of_conf);
    
    if (ms_since_conf > m_device_ready_timeout)
    {
      throw TimingEndpointNotReady(ERS_HERE,"HSI ("+m_timing_device+")", m_endpoint_state);
    }
    TLOG_DEBUG(3) << "Waiting for HSI endpoint to become ready for (ms) " << ms_since_conf.count();
    std::this_thread::sleep_for(std::chrono::microseconds(250000));
  }
  TLOG() << get_name() << " conf; hsi device: " << m_timing_device;
}

void
HSIController::do_start(const nlohmann::json& data)
{
  TimingController::do_start(data); // set sent cmd counters to 0

  auto start_params = data.get<rcif::cmd::StartParams>();
  if (start_params.trigger_rate > 0)
  {
    m_hsi_configuration.trigger_rate = start_params.trigger_rate;
    TLOG() << get_name() << " Changing rate: trigger_rate "
           << m_hsi_configuration.trigger_rate;

    do_hsi_reset(data);
    do_hsi_configure(m_hsi_configuration);
  }
  do_hsi_start(m_hsi_configuration);
}

void
HSIController::do_stop(const nlohmann::json& data)
{
  do_hsi_stop(data);
}

void
HSIController::do_change_rate(const nlohmann::json& data)
{
  auto change_rate_params = data.get<rcif::cmd::ChangeRateParams>();
  m_hsi_configuration.trigger_rate = change_rate_params.trigger_rate;
  TLOG() << get_name() << " Changing rate: trigger_rate "
         << m_hsi_configuration.trigger_rate;

  do_hsi_configure(m_hsi_configuration);
}

timingcmd::TimingHwCmd
HSIController::construct_hsi_hw_cmd(const std::string& cmd_id)
{
    timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
HSIController::do_hsi_io_reset(const nlohmann::json& data)
{
  auto hw_cmd = construct_hsi_hw_cmd("io_reset");
  hw_cmd.payload = data;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
HSIController::do_hsi_endpoint_enable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd("endpoint_enable");

  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timingcmd::from_json(data, cmd_payload);

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
HSIController::do_hsi_endpoint_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "endpoint_disable");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
HSIController::do_hsi_endpoint_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "endpoint_reset");

  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timingcmd::from_json(data, cmd_payload);

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
HSIController::do_hsi_reset(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "hsi_reset");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
HSIController::do_hsi_configure(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd("hsi_configure");
  hw_cmd.payload = data;

  double trigger_rate = data["trigger_rate"];

  if (trigger_rate <= 0) {
    ers::error(InvalidTriggerRateValue(ERS_HERE, trigger_rate));
    return;
  }

  hw_cmd.payload["random_rate"] = trigger_rate;

  TLOG() << get_name() << " Setting emulated event rate [Hz] to: "
         << trigger_rate;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
HSIController::do_hsi_start(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "hsi_start");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
HSIController::do_hsi_stop(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "hsi_stop");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(7).atomic);
}

void
HSIController::do_hsi_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hsi_hw_cmd( "hsi_print_status");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(8).atomic);
}

void
HSIController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  hsicontrollerinfo::Info module_info;
  module_info.sent_hsi_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_hsi_endpoint_enable_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_hsi_endpoint_disable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  module_info.sent_hsi_endpoint_reset_cmds = m_sent_hw_command_counters.at(3).atomic.load();
  module_info.sent_hsi_reset_cmds = m_sent_hw_command_counters.at(4).atomic.load();
  module_info.sent_hsi_configure_cmds = m_sent_hw_command_counters.at(5).atomic.load();
  module_info.sent_hsi_start_cmds = m_sent_hw_command_counters.at(6).atomic.load();
  module_info.sent_hsi_stop_cmds = m_sent_hw_command_counters.at(7).atomic.load();
  module_info.sent_hsi_print_status_cmds = m_sent_hw_command_counters.at(8).atomic.load();
  module_info.device_infos_received_count = m_device_infos_received_count;

  ci.add(module_info);
}

void
HSIController::process_device_info(nlohmann::json info)
{
  timing::timingfirmwareinfo::HSIFirmwareMonitorData hsi_info;

  auto hsi_data = info[opmonlib::JSONTags::children]["hsi"][opmonlib::JSONTags::properties][hsi_info.info_type][opmonlib::JSONTags::data];

  from_json(hsi_data, hsi_info);

  m_endpoint_state = hsi_info.endpoint_state;
  
  TLOG_DEBUG(3) << "HSI ept state: 0x" << std::hex << m_endpoint_state;

  if (hsi_info.endpoint_state == 0x8)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "HSI endpoint became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "HSI endpoint no longer ready";
    }
  }
  ++m_device_infos_received_count;
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::HSIController)

// Local Variables:
// c-basic-offset: 2
// End:
