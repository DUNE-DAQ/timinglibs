/**
 * @file TimingFanoutController.cpp TimingFanoutController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingFanoutController.hpp"
#include "timinglibs/dal/TimingFanoutController.hpp"

#include "timinglibs/timingfanoutcontroller/Nljs.hpp"
#include "timinglibs/timingfanoutcontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

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
  : dunedaq::timinglibs::TimingController(name, 4) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingFanoutController::do_configure);
  register_command("start", &TimingFanoutController::do_start);
  register_command("stop", &TimingFanoutController::do_stop);
  register_command("scrap", &TimingFanoutController::do_scrap);
  
  // timing fanout hardware commands
  register_command("fanout_io_reset", &TimingFanoutController::do_fanout_io_reset);
  register_command("fanout_print_status", &TimingFanoutController::do_fanout_print_status);
  register_command("fanout_endpoint_enable", &TimingFanoutController::do_fanout_endpoint_enable);
  register_command("fanout_endpoint_reset", &TimingFanoutController::do_fanout_endpoint_reset);
}

void
TimingFanoutController::do_configure(const nlohmann::json& data)
{
  auto mdal = m_params->cast<dal::TimingFanoutController>(); 
  if (mdal->get_device().empty())
  {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name should not be empty");
  }
  
  m_device_ready_timeout = std::chrono::milliseconds(mdal->get_device_ready_timeout());

  TimingController::do_configure(data); // configure hw command connection

  configure_hardware_or_recover_state<TimingFanoutNotReady>(data, "Timing fanout");

  TLOG() << get_name() << "conf done for fanout device: " << m_timing_device;
}

// TODO: CHANGE
void
TimingFanoutController::send_configure_hardware_commands(const nlohmann::json& data)
{
  do_fanout_io_reset(data);
  std::this_thread::sleep_for(std::chrono::milliseconds(15000));
  do_fanout_endpoint_reset(data);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

// TODO: CHANGE
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
TimingFanoutController::do_fanout_endpoint_enable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "endpoint_enable";
  hw_cmd.device = m_timing_device;

  // make our hw cmd
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  cmd_payload.address = 0;
  cmd_payload.partition = 0; 

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << "fanout ept enable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingFanoutController::do_fanout_endpoint_reset(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "endpoint_reset";
  hw_cmd.device = m_timing_device;

  // make our hw cmd
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  cmd_payload.address = 0;
  cmd_payload.partition = 0;

  TLOG_DEBUG(0) << "fanout ept reset hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

// void
// TimingFanoutController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
// {
//   // send counters internal to the module
//   timingfanoutcontrollerinfo::Info module_info;

//   module_info.sent_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
//   module_info.sent_print_status_cmds = m_sent_hw_command_counters.at(1).atomic.load();
//   module_info.sent_fanout_endpoint_enable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
//   module_info.sent_fanout_endpoint_reset_cmds = m_sent_hw_command_counters.at(3).atomic.load();

//   ci.add(module_info);
// }

// void
// TimingFanoutController::process_device_info(nlohmann::json info)
// {
//   ++m_device_infos_received_count;

//   timing::timingendpointinfo::TimingEndpointInfo ept_info;

//   auto ept_data = info[opmonlib::JSONTags::children]["endpoint"][opmonlib::JSONTags::properties][ept_info.info_type][opmonlib::JSONTags::data];

//   from_json(ept_data, ept_info);

//   uint32_t endpoint_state = ept_info.state;
//   bool ready = ept_info.ready;

//   TLOG_DEBUG(3) << "state: 0x" << std::hex << endpoint_state << ", ready: " << ready << std::dec << ", infos received: " << m_device_infos_received_count;;

//   if (endpoint_state > 0x5 && endpoint_state < 0x9)
//   {
//     if (!m_device_ready)
//     {
//       m_device_ready = true;
//       TLOG_DEBUG(2) << "Timing fanout became ready";
//     }
//   }
//   else
//   {
//     if (m_device_ready)
//     {
//       m_device_ready = false;
//       TLOG_DEBUG(2) << "Timing fanout no longer ready";
//     }
//   }
// }
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingFanoutController)

// Local Variables:
// c-basic-offset: 2
// End:
