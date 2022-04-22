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

#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"

#include "serialization/Serialization.hpp"
#include "ipm/Receiver.hpp"
#include "networkmanager/NetworkManager.hpp"

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

TimingPartitionController::TimingPartitionController(const std::string& name)
  : dunedaq::timinglibs::TimingController(name, 8)// 2nd arg: how many hw commands can this module send?
  , m_partition_trigger_mask(0x0)
  , m_partition_control_rate_enabled(false)
  , m_partition_spill_gate_enabled(false)
 
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
  m_hw_command_connection =  conf.hw_cmd_connection;
  
  m_timing_device = conf.device;
  m_managed_partition_id = conf.partition_id;
  
  // parameters against which to compare partition state
  m_partition_trigger_mask = conf.trigger_mask;
  m_partition_control_rate_enabled = conf.rate_control_enabled;
  m_partition_spill_gate_enabled = conf.spill_gate_enabled;

  TimingController::do_configure(data); // configure hw command connection

  do_partition_configure(data);
  do_partition_enable(data);

  auto time_of_conf = std::chrono::high_resolution_clock::now();
  while (!m_device_ready)
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms_since_conf = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_of_conf);
    
    if (ms_since_conf > m_device_ready_timeout)
    {
      throw TimingPartitionNotReady(ERS_HERE, m_timing_device, m_managed_partition_id);
    }
    TLOG_DEBUG(3) << "Waiting for timing partition " << m_managed_partition_id << " to become ready for (ms) " << ms_since_conf.count();
    std::this_thread::sleep_for(std::chrono::microseconds(250000));
  }
  TLOG() << get_name() << " conf; device: " << m_timing_device << ", managed part id: " << m_managed_partition_id;
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

void
TimingPartitionController::do_scrap(const nlohmann::json& data)
{
  TimingController::do_scrap(data); // configure hw command connection
  do_partition_disable(data);
}

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

void
TimingPartitionController::construct_partition_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  cmd_payload.partition_id = m_managed_partition_id;
  timingcmd::to_json(hw_cmd.payload, cmd_payload);

  hw_cmd.id = cmd_id;
  hw_cmd.device = m_timing_device;
}

void
TimingPartitionController::do_partition_configure(const nlohmann::json& data)
{
  timingcmd::TimingHwCmd hw_cmd;
  hw_cmd.id = "partition_configure";
  hw_cmd.device = m_timing_device;
  hw_cmd.payload = data;

  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(0).atomic);
}

void
TimingPartitionController::do_partition_enable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_enable");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(1).atomic);
}

void
TimingPartitionController::do_partition_disable(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_disable");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(2).atomic);
}

void
TimingPartitionController::do_partition_start(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_start");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(3).atomic);
}

void
TimingPartitionController::do_partition_stop(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_stop");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(4).atomic);
}

void
TimingPartitionController::do_partition_enable_triggers(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_enable_triggers");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(5).atomic);
}

void
TimingPartitionController::do_partition_disable_triggers(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_disable_triggers");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(6).atomic);
}

void
TimingPartitionController::do_partition_print_status(const nlohmann::json&)
{
  timingcmd::TimingHwCmd hw_cmd;
  construct_partition_hw_cmd(hw_cmd, "partition_print_status");
  send_hw_cmd(hw_cmd);
  ++(m_sent_hw_command_counters.at(7).atomic);
}

void
TimingPartitionController::process_device_info(ipm::Receiver::Response message)
{
  auto data = nlohmann::json::from_msgpack(message.data);  

  timing::timingfirmwareinfo::TimingPartitionMonitorData partition_info;

  std::string partition_label = "partition"+std::to_string(m_managed_partition_id);
  
  auto partition_data = data[opmonlib::JSONTags::children]["master"]
                            [opmonlib::JSONTags::children][partition_label]
                            [opmonlib::JSONTags::properties][partition_info.info_type][opmonlib::JSONTags::data];

  from_json(partition_data, partition_info);

  bool partition_enabled = partition_info.enabled;
  bool partition_rate_control_enabled = partition_info.rate_ctrl_enabled;
  uint16_t partition_trigger_mask = partition_info.trig_mask;

  TLOG_DEBUG(3) << "Partition enabled: " << partition_enabled;
  TLOG_DEBUG(3) << "Partition rate control enabled: " << partition_enabled;
  TLOG_DEBUG(3) << "Partition trigger mask: 0x" << std::hex << partition_trigger_mask;

  if (partition_enabled && partition_rate_control_enabled == m_partition_control_rate_enabled && partition_trigger_mask == m_partition_trigger_mask)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "Timing partition became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "Timing partition no longer ready";
    }
  }
  ++m_device_infos_received_count;
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
