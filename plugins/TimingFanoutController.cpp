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

  // timing fanout hardware commands
  register_command("fanout_io_reset", &TimingFanoutController::do_fanout_io_reset);
  register_command("fanout_print_status", &TimingFanoutController::do_fanout_print_status);
}

void
TimingFanoutController::init(const nlohmann::json& init_data)
{
  // set up queues
  TimingController::init(init_data["qinfos"]);

  auto ini = init_data.get<timingfanoutcontroller::InitParams>();
  m_timing_device = ini.device;

  TLOG() << get_name() << "conf: fanout device: " << m_timing_device;
}

void
TimingFanoutController::do_configure(const nlohmann::json& obj)
{
}

void
TimingFanoutController::construct_fanout_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id)
{
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
}

void
TimingFanoutController::do_fanout_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_fanout_hw_cmd(hw_cmd, "io_reset");
  hw_cmd.payload = data;
  hw_cmd.payload["fanout_mode"] = 0; // fanout mode for fanout design

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingFanoutController::do_fanout_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_fanout_hw_cmd(hw_cmd, "print_status");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingFanoutController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  timingfanoutcontrollerinfo::Info module_info;
  
  module_info.sent_io_reset_cmds=m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_print_status_cmds=m_sent_hw_command_counters.at(1).atomic.load();
  
  //for (uint i = 0; i < m_number_hw_commands; ++i) {
  //  module_info.sent_hw_command_counters.push_back(m_sent_hw_command_counters.at(i).atomic.load());
  //}
  ci.add(module_info);
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingFanoutController)

// Local Variables:
// c-basic-offset: 2
// End:
