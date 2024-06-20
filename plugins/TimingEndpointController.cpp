/**
 * @file TimingEndpointController.cpp TimingEndpointController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

// #include "timinglibs/dal/TimingEndpointController.hpp"
#include "TimingEndpointController.hpp"
#include "timinglibs/dal/TimingEndpointController.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingendpointcontroller/Nljs.hpp"
#include "timinglibs/timingendpointcontroller/Structs.hpp"

#include "timing/timingendpointinfo/InfoNljs.hpp"
#include "timing/timingendpointinfo/InfoStructs.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingEndpointController::TimingEndpointController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 6) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingEndpointController::do_configure);
  register_command("start", &TimingEndpointController::do_start);
  register_command("stop", &TimingEndpointController::do_stop);
  register_command("scrap", &TimingEndpointController::do_scrap);

  // timing endpoint hardware commands
  register_command("endpoint_io_reset", &TimingEndpointController::do_endpoint_io_reset);
  register_command("endpoint_enable", &TimingEndpointController::do_endpoint_enable);
  register_command("endpoint_disable", &TimingEndpointController::do_endpoint_disable);
  register_command("endpoint_reset", &TimingEndpointController::do_endpoint_reset);
  register_command("endpoint_print_status", &TimingEndpointController::do_endpoint_print_status);
  register_command("endpoint_print_timestamp", &TimingEndpointController::do_endpoint_print_timestamp);
}

void
TimingEndpointController::do_configure(const nlohmann::json& data)
{  
  auto mdal = m_params->cast<dal::TimingEndpointController>(); 

  if (mdal->get_device().empty()) {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name should not be empty");
  }
  
  m_managed_endpoint_id = mdal->get_endpoint_id();

  TimingController::do_configure(data); // configure hw command connection

  configure_hardware_or_recover_state<TimingEndpointNotReady>(data, "Timing endpoint", m_endpoint_state);

  TLOG() << get_name() << " conf done for endpoint, device: " << m_timing_device;
}

void
TimingEndpointController::send_configure_hardware_commands(const nlohmann::json& data)
{
  do_endpoint_io_reset(data);
  std::this_thread::sleep_for(std::chrono::microseconds(7000000));
  do_endpoint_enable(data);
}

// TODO: CHANGE
timingcmd::TimingHwCmd
TimingEndpointController::construct_endpoint_hw_cmd( const std::string& cmd_id)
{
  timingcmd::TimingHwCmd hw_cmd;
  timingcmd::TimingEndpointCmdPayload cmd_payload;
  cmd_payload.endpoint_id = m_managed_endpoint_id;
  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
TimingEndpointController::do_endpoint_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_endpoint_hw_cmd( "io_reset");
  hw_cmd.payload = data;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingEndpointController::do_endpoint_enable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "endpoint_enable";
  hw_cmd.device = m_timing_device;

  // make our hw cmd
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = m_managed_endpoint_id;
  timingcmd::from_json(data, cmd_payload);

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingEndpointController::do_endpoint_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_endpoint_hw_cmd( "endpoint_disable");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingEndpointController::do_endpoint_reset(const nlohmann::json& data)
{
    timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "endpoint_reset";
  hw_cmd.device = m_timing_device;

  // make our hw cmd
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = m_managed_endpoint_id;
  timingcmd::from_json(data, cmd_payload);

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingEndpointController::do_endpoint_print_timestamp(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_endpoint_hw_cmd( "endpoint_print_timestamp");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
TimingEndpointController::do_endpoint_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_endpoint_hw_cmd( "print_status");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(5).atomic);
}

// TODO: CHANGE
void
TimingEndpointController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{

  // send counters internal to the module
  timingendpointcontrollerinfo::Info module_info;
  module_info.sent_endpoint_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_endpoint_enable_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_endpoint_disable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  module_info.sent_endpoint_reset_cmds = m_sent_hw_command_counters.at(3).atomic.load();
  module_info.sent_endpoint_print_status_cmds = m_sent_hw_command_counters.at(4).atomic.load();
  module_info.sent_endpoint_print_timestamp_cmds = m_sent_hw_command_counters.at(5).atomic.load();
  ci.add(module_info);
}

void
TimingEndpointController::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;
  
  timing::timingendpointinfo::TimingEndpointInfo ept_info;

  auto ept_data = info[opmonlib::JSONTags::children]["endpoint"][opmonlib::JSONTags::properties][ept_info.info_type][opmonlib::JSONTags::data];

  from_json(ept_data, ept_info);

  m_endpoint_state = ept_info.state;
  bool ready = ept_info.ready;

  TLOG_DEBUG(3) << "state: 0x" << std::hex << m_endpoint_state << ", ready: " << ready << std::dec << ", infos received: " << m_device_infos_received_count;

  if (m_endpoint_state == 0x8 && ready)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "Timing endpoint became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "Timing endpoint no longer ready";
    }
  }
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingEndpointController)

// Local Variables:
// c-basic-offset: 2
// End: