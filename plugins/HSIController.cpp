/**
 * @file HSIController.cpp HSIController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIController.hpp"

#include "timinglibs/hsicontroller/Nljs.hpp"
#include "timinglibs/hsicontroller/Structs.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
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

HSIController::HSIController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 9) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &HSIController::do_configure);
  register_command("start", &HSIController::do_start);
  register_command("stop", &HSIController::do_stop);

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
HSIController::init(const nlohmann::json& init_data)
{
  // set up queues
  TimingController::init(init_data["qinfos"]);
  
  auto ini = init_data.get<hsicontroller::InitParams>();
  m_timing_device = ini.device;
  
  TLOG() << get_name() << " init; hsi device: " << m_timing_device;
}

void
HSIController::do_configure(const nlohmann::json& data)
{
  std::this_thread::sleep_for(std::chrono::microseconds(5000000));

  do_hsi_reset(data);
  do_hsi_endpoint_reset(data);
  do_hsi_configure(data);
}

void
HSIController::do_start(const nlohmann::json& data)
{
  TimingController::do_start(data); // set sent cmd counters to 0
  do_hsi_start(data);
}

void
HSIController::do_stop(const nlohmann::json& data)
{
  do_hsi_stop(data);
}

void
HSIController::construct_hsi_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id)
{
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
}

void
HSIController::do_hsi_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "io_reset");
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
HSIController::do_hsi_endpoint_enable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "endpoint_enable");

  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timingcmd::from_json(data, cmd_payload);

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << "ept enable hw cmd; a: " << cmd_payload.address << ", p: " << cmd_payload.partition;
  
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
HSIController::do_hsi_endpoint_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "endpoint_disable");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
HSIController::do_hsi_endpoint_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "endpoint_reset");
  
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  cmd_payload.endpoint_id = 0;
  timingcmd::from_json(data, cmd_payload);

  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
HSIController::do_hsi_reset(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "hsi_reset");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
HSIController::do_hsi_configure(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "hsi_configure");
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
HSIController::do_hsi_start(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "hsi_start");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
HSIController::do_hsi_stop(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "hsi_stop");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(7).atomic);
}

void
HSIController::do_hsi_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_hsi_hw_cmd(hw_cmd, "print_status");
  send_hw_cmd(hw_cmd);
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
  ci.add(module_info);
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::HSIController)

// Local Variables:
// c-basic-offset: 2
// End:
