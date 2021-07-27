/**
 * @file TimingEndpointController.cpp TimingEndpointController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingEndpointController.hpp"

#include "timinglibs/timingendpointcontroller/Nljs.hpp"
#include "timinglibs/timingendpointcontroller/Structs.hpp"

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

TimingEndpointController::TimingEndpointController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 6) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingEndpointController::do_configure);
  register_command("start", &TimingEndpointController::do_start);
  register_command("stop", &TimingEndpointController::do_stop);

  // timing endpoint hardware commands
  register_command("endpoint_io_reset", &TimingEndpointController::do_endpoint_io_reset);
  register_command("endpoint_enable", &TimingEndpointController::do_endpoint_enable);
  register_command("endpoint_disable", &TimingEndpointController::do_endpoint_disable);
  register_command("endpoint_reset", &TimingEndpointController::do_endpoint_reset);
  register_command("endpoint_print_status", &TimingEndpointController::do_endpoint_print_status);
  register_command("endpoint_print_timestamp", &TimingEndpointController::do_endpoint_print_timestamp);
}

void
TimingEndpointController::do_configure(const nlohmann::json& obj)
{
  timingendpointcontroller::from_json(obj, m_cfg);

  TLOG_DEBUG(0) << get_name() << " conf: managed endpoint, device: " << m_cfg.device;
}

void
TimingEndpointController::construct_endpoint_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id)
{
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_cfg.device;
}

void
TimingEndpointController::do_endpoint_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "io_reset");
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingEndpointController::do_endpoint_enable(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "endpoint_enable");
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingEndpointController::do_endpoint_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "endpoint_disable");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingEndpointController::do_endpoint_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "endpoint_reset");
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingEndpointController::do_endpoint_print_timestamp(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "endpoint_print_timestamp");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
TimingEndpointController::do_endpoint_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_endpoint_hw_cmd(hw_cmd, "print_status");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(5).atomic);
}

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
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingEndpointController)

// Local Variables:
// c-basic-offset: 2
// End:
