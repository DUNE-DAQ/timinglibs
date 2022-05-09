/**
 * @file TimingPartitionController.cpp TimingPartitionController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingPartitionController.hpp"
#include "timinglibs/timingpartitioncontroller/Nljs.hpp"
#include "timinglibs/timingpartitioncontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "ers/Issue.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingPartitionController::TimingPartitionController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 8) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingPartitionController::do_configure);
  register_command("start", &TimingPartitionController::do_start);
  register_command("stop", &TimingPartitionController::do_stop);
  register_command("resume", &TimingPartitionController::do_resume);
  register_command("pause", &TimingPartitionController::do_pause);
//  register_command("scrap", &TimingPartitionController::do_scrap);

  // timing partition hw commands
  register_command("partition_configure", &TimingPartitionController::do_partition_configure);
  register_command("partition_enable", &TimingPartitionController::do_partition_enable);
  register_command("partition_disable", &TimingPartitionController::do_partition_disable);
  register_command("partition_start", &TimingPartitionController::do_partition_start);
  register_command("partition_stop", &TimingPartitionController::do_partition_stop);
  register_command("partition_enable_triggers", &TimingPartitionController::do_partition_enable_triggers);
  register_command("partition_disable_triggers", &TimingPartitionController::do_partition_disable_triggers);
  register_command("partition_print_status", &TimingPartitionController::do_partition_print_status);
}

void
TimingPartitionController::do_configure(const nlohmann::json& data)
{
  auto conf = data.get<timingpartitioncontroller::PartitionConfParams>();
  if (conf.device.empty())
  {
    throw UHALDeviceNameIssue(ERS_HERE, "Device name should not be empty");
  }
  m_timing_device = conf.device;
  m_managed_partition_id = conf.partition_id;

  TLOG() << get_name() << " conf; device: " << m_timing_device << ", managed part id: " << m_managed_partition_id;
  do_partition_configure(data);
  do_partition_enable(data);
}

void
TimingPartitionController::do_start(const nlohmann::json& data)
{
  TimingController::do_start(data); // set sent cmd counters to 0
  do_partition_start(data);
}

void
TimingPartitionController::do_stop(const nlohmann::json& data)
{
  do_partition_stop(data);
}

//void
//TimingPartitionController::do_scrap(const nlohmann::json& data)
//{
//  do_partition_disable(data);
//}

void
TimingPartitionController::do_resume(const nlohmann::json& data)
{
  do_partition_enable_triggers(data);
}

void
TimingPartitionController::do_pause(const nlohmann::json& data)
{
  do_partition_disable_triggers(data);
}

timingcmd::TimingHwCmd
TimingPartitionController::construct_partition_hw_cmd(const std::string& cmd_id)
{
    timingcmd::TimingHwCmd hw_cmd;
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  cmd_payload.partition_id = m_managed_partition_id;
  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
  return hw_cmd;
}

void
TimingPartitionController::do_partition_configure(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "partition_configure";
  hw_cmd.device = m_timing_device;
  hw_cmd.payload = data;

  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingPartitionController::do_partition_enable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_enable");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingPartitionController::do_partition_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_disable");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingPartitionController::do_partition_start(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_start");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingPartitionController::do_partition_stop(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_stop");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
TimingPartitionController::do_partition_enable_triggers(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_enable_triggers");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
TimingPartitionController::do_partition_disable_triggers(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd =
  construct_partition_hw_cmd( "partition_disable_triggers");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
TimingPartitionController::do_partition_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd = 
  construct_partition_hw_cmd( "partition_print_status");
  send_hw_cmd(std::move(hw_cmd));
  ++(m_sent_hw_command_counters.at(7).atomic);
}

void
TimingPartitionController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  timingpartitioncontrollerinfo::Info module_info;
  module_info.sent_partition_configure_cmds = m_sent_hw_command_counters.at(0).atomic.load();
  module_info.sent_partition_enable_cmds = m_sent_hw_command_counters.at(1).atomic.load();
  module_info.sent_partition_disable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
  module_info.sent_partition_start_cmds = m_sent_hw_command_counters.at(3).atomic.load();
  module_info.sent_partition_stop_cmds = m_sent_hw_command_counters.at(4).atomic.load();
  module_info.sent_partition_enable_triggers_cmds = m_sent_hw_command_counters.at(5).atomic.load();
  module_info.sent_partition_disable_triggers_cmds = m_sent_hw_command_counters.at(6).atomic.load();
  module_info.sent_partition_print_status_cmds = m_sent_hw_command_counters.at(7).atomic.load();

  // for (uint i = 0; i < m_number_hw_commands; ++i) {
  //  module_info.sent_hw_command_counters.push_back(m_sent_hw_command_counters.at(i).atomic.load());
  //}
  ci.add(module_info);
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingPartitionController)

// Local Variables:
// c-basic-offset: 2
// End:
