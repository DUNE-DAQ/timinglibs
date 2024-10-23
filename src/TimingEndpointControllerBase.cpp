/**
 * @file TimingEndpointControllerBase.cpp TimingEndpointControllerBase class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "timinglibs/TimingEndpointControllerBase.hpp"
#include "timinglibs/dal/TimingEndpointControllerConf.hpp"
#include "timinglibs/dal/TimingEndpointControllerBase.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingfirmwareinfo/Nljs.hpp"
#include "timing/timingfirmwareinfo/Structs.hpp"

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

TimingEndpointControllerBase::TimingEndpointControllerBase(const std::string& name, uint number_hw_commands)
  : dunedaq::timinglibs::TimingController(name, number_hw_commands) // 2nd arg: how many hw commands can this module send?
{
  // timing endpoint hardware commands
  register_command("endpoint_enable", &TimingEndpointControllerBase::do_endpoint_enable);
  register_command("endpoint_disable", &TimingEndpointControllerBase::do_endpoint_disable);
  register_command("endpoint_reset", &TimingEndpointControllerBase::do_endpoint_reset);
}

void
TimingEndpointControllerBase::do_configure(const nlohmann::json& data)
{
  auto mdal = m_params->cast<dal::TimingEndpointControllerConf>();

  TimingController::do_configure(data); // configure hw command connection

  // endpoint per device in config for now...
  m_managed_endpoint_id = {mdal->get_endpoint_id()};

  configure_hardware_or_recover_state<TimingEndpointNotReady>(data, "Timing endpoint", m_endpoint_state);

  TLOG() << get_name() << " conf done for endpoint, device: " << m_timing_device;
}

void
TimingEndpointControllerBase::send_configure_hardware_commands(const nlohmann::json& data)
{
  do_io_reset(data);
  std::this_thread::sleep_for(std::chrono::microseconds(7000000));
  do_endpoint_enable(data);
}

timingcmd::TimingHwCmd
TimingEndpointControllerBase::construct_endpoint_hw_cmd( const std::string& cmd_id, uint endpoint_id)
{
  timingcmd::TimingHwCmd hw_cmd;
  timingcmd::TimingEndpointCmdPayload cmd_payload;
  // where to check endpoint id validity
  cmd_payload.endpoint_id = endpoint_id;
  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
TimingEndpointControllerBase::do_endpoint_enable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hw_cmd( "endpoint_enable", data);

  // print out some debug info
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(data, cmd_payload);

  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << cmd_payload.address;
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingEndpointControllerBase::do_endpoint_disable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hw_cmd( "endpoint_disable", data);
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingEndpointControllerBase::do_endpoint_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_hw_cmd( "endpoint_reset", data);

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(4).atomic);
}

//void
//TimingEndpointControllerBase::get_info(opmonlib::InfoCollector& ci, int /*level*/)
//{

  // send counters internal to the module
//  timingendpointcontrollerinfo::Info module_info;
//  module_info.sent_endpoint_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
//  module_info.sent_endpoint_print_status_cmds = m_sent_hw_command_counters.at(1).atomic.load();

//  module_info.sent_endpoint_enable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
//  module_info.sent_endpoint_disable_cmds = m_sent_hw_command_counters.at(3).atomic.load();
//  module_info.sent_endpoint_reset_cmds = m_sent_hw_command_counters.at(4).atomic.load();
//  module_info.sent_endpoint_print_timestamp_cmds = m_sent_hw_command_counters.at(5).atomic.load();
//  ci.add(module_info);
//}

void
TimingEndpointControllerBase::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;
  
  timing::timingfirmwareinfo::TimingDeviceInfo device_info;
  from_json(info, device_info);

  auto ept_info = device_info.endpoint_info;

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

// Local Variables:
// c-basic-offset: 2
// End:
