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
  : dunedaq::timinglibs::TimingController(name, 3) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingMasterController::do_configure);
  register_command("start", &TimingMasterController::do_start);
  register_command("stop", &TimingMasterController::do_stop);

  // timing master hardware commands
  register_command("master_io_reset", &TimingMasterController::do_master_io_reset);
  register_command("master_set_timestamp", &TimingMasterController::do_master_set_timestamp);
  register_command("master_print_status", &TimingMasterController::do_master_print_status);
}

void
TimingMasterController::init(const nlohmann::json& init_data)
{
  // set up queues
  TimingController::init(init_data["qinfos"]);
  
  auto ini = init_data.get<timingmastercontroller::InitParams>();
  m_timing_device = ini.device;
  
  TLOG() << get_name() << " init: master, device: " << m_timing_device;
}

void
TimingMasterController::do_configure(const nlohmann::json& obj)
{
}

void
TimingMasterController::construct_master_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id)
{
  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
}

void
TimingMasterController::do_master_io_reset(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_master_hw_cmd(hw_cmd, "io_reset");
  hw_cmd.payload = data;
  hw_cmd.payload["fanout_mode"] = 1; // put hw in standalone if fanout design

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingMasterController::do_master_set_timestamp(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_master_hw_cmd(hw_cmd, "set_timestamp");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingMasterController::do_master_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_master_hw_cmd(hw_cmd, "print_status");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingMasterController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  timingmastercontrollerinfo::Info module_info;
  module_info.sent_master_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_master_set_timestamp_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_master_print_status_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  
  //for (uint i = 0; i < m_number_hw_commands; ++i) {
  //  module_info.sent_hw_command_counters.push_back(m_sent_hw_command_counters.at(i).atomic.load());
  //}
  ci.add(module_info);
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingMasterController)

// Local Variables:
// c-basic-offset: 2
// End:
