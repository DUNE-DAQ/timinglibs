/**
 * @file TimingHardwareManager.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingHardwareManager.hpp"
#include "logging/Logging.hpp"

#include <memory>
#include <utility>
#include <string>

#define TRACE_NAME "TimingHardwareManager" // NOLINT

namespace dunedaq {
namespace timinglibs {

TimingHardwareManager::TimingHardwareManager(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , thread_(std::bind(&TimingHardwareManager::process_hardware_commands, this, std::placeholders::_1))
  , m_hw_command_in_queue(nullptr)
  , m_queue_timeout(100)
  , m_gather_interval(1e6)
  , m_gather_interval_debug(10e6)
  , m_connections_file("")
  , m_uhal_log_level("notice")
  , m_connection_manager(nullptr)
  , m_received_hw_commands_counter{ 0 }
  , m_accepted_hw_commands_counter{ 0 }
  , m_rejected_hw_commands_counter{ 0 }
  , m_failed_hw_commands_counter{ 0 }
{
  //  register_command("start", &TimingHardwareManager::do_start);
  //  register_command("stop", &TimingHardwareManager::do_stop);
  //  register_command("scrap", &TimingHardwareManager::do_scrap);
}

void
TimingHardwareManager::init(const nlohmann::json& init_data)
{
  // set up queues
  auto qinfos = init_data.get<appfwk::app::QueueInfos>();
  for (const auto& qi : qinfos) {
    if (!qi.name.compare("hardware_commands_in")) {
      try {
        m_hw_command_in_queue.reset(new source_t(qi.inst));
      } catch (const ers::Issue& excpt) {
        throw InvalidQueueFatalError(ERS_HERE, get_name(), qi.name, excpt);
      }
    }
  }
  m_received_hw_commands_counter = 0;
  m_accepted_hw_commands_counter = 0;
  m_rejected_hw_commands_counter = 0;
  m_failed_hw_commands_counter = 0;
}

const timing::TimingNode*
TimingHardwareManager::get_timing_device_plain(const std::string& device_name)
{

  if (!device_name.compare("")) {
    std::stringstream message;
    message << "UHAL device name is an empty string";
    throw UHALDeviceNameIssue(ERS_HERE, message.str());
  }

  std::lock_guard<std::mutex> hw_device_map_guard(m_hw_device_map_mutex);

  if (auto hw_device_entry = m_hw_device_map.find(device_name); hw_device_entry != m_hw_device_map.end()) {
    return dynamic_cast<const timing::TimingNode*>(&hw_device_entry->second->getNode(""));
  } else {
    TLOG_DEBUG(0) << get_name() << ": hw device interface for: " << device_name
                  << " does not exist. I will try to create it.";

    try {
      m_hw_device_map.emplace(device_name,
                              std::make_unique<uhal::HwInterface>(m_connection_manager->getDevice(device_name)));
    } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
      std::stringstream message;
      message << "UHAL device name not " << device_name << " in connections file";
      throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
    }

    TLOG_DEBUG(0) << get_name() << ": hw device interface for: " << device_name << " successfully created.";

    return dynamic_cast<const timing::TimingNode*>(&m_hw_device_map.find(device_name)->second->getNode(""));
  }
}

void
TimingHardwareManager::gather_monitor_data(InfoGatherer& gatherer)
{
  auto device_name = gatherer.get_device_name();

  while (gatherer.run_gathering()) {

    // collect the data from the hardware
    try {
      auto design = get_timing_device_plain(device_name);
      gatherer.collect_info_from_device(*design);
      gatherer.update_last_gathered_time(std::time(nullptr));
    } catch (const std::exception& excpt) {
      ers::warning(FailedToCollectOpMonInfo(ERS_HERE, device_name, excpt));
    }

    auto prev_gather_time = std::chrono::steady_clock::now();
    auto next_gather_time = prev_gather_time + std::chrono::microseconds(gatherer.get_gather_interval());

    // check running_flag periodically
    auto slice_period = std::chrono::microseconds(10000);
    auto next_slice_gather_time = prev_gather_time + slice_period;

    bool break_flag = false;
    while (next_gather_time > next_slice_gather_time + slice_period) {
      if (!gatherer.run_gathering()) {
        TLOG_DEBUG(0) << "while waiting to gather data, negative run gatherer flag detected.";
        break_flag = true;
        break;
      }
      std::this_thread::sleep_until(next_slice_gather_time);
      next_slice_gather_time = next_slice_gather_time + slice_period;
    }
    if (break_flag == false) {
      std::this_thread::sleep_until(next_gather_time);
    }
  }
}

void
TimingHardwareManager::register_info_gatherer(uint gather_interval, const std::string& device_name, int op_mon_level)
{
  std::string gatherer_name = device_name + "_level_" + std::to_string(op_mon_level);
  if (m_info_gatherers.find(gatherer_name) == m_info_gatherers.end()) {
    std::unique_ptr<InfoGatherer> gatherer = std::make_unique<InfoGatherer>(
      std::bind(&TimingHardwareManager::gather_monitor_data, this, std::placeholders::_1),
      gather_interval,
      device_name,
      op_mon_level);

    TLOG_DEBUG(0) << "Registering info gatherer: " << gatherer_name;
    m_info_gatherers.emplace(std::make_pair(gatherer_name, std::move(gatherer)));
  } else {
    TLOG() << "Skipping registration of " << gatherer_name << ". Already exists.";
  }
}

void
TimingHardwareManager::start_hw_mon_gathering(const std::string& device_name)
{
  // start all gatherers if no device name is given
  if (!device_name.compare("")) {
    TLOG_DEBUG(0) << get_name() << " Starting all info gatherers";
    for (auto it = m_info_gatherers.begin(); it != m_info_gatherers.end(); ++it)
      it->second.get()->start_gathering_thread();
  } else {
    // find gatherer for suppled device name and start it
    bool gatherer_found=false;
    for (auto it = m_info_gatherers.lower_bound(device_name); it != m_info_gatherers.end(); ++it) {
      TLOG_DEBUG(0) << get_name() << " Starting info gatherer: " << it->first;
      it->second.get()->start_gathering_thread();
      gatherer_found=true;
    } 
    if (!gatherer_found) ers::warning(AttemptedToControlNonExantInfoGatherer(ERS_HERE, "start", device_name));
  }
}

void
TimingHardwareManager::stop_hw_mon_gathering(const std::string& device_name)
{
  // stop all gatherers if no device name is given
  if (!device_name.compare("")) {
    TLOG_DEBUG(0) << get_name() << " Stopping all info gatherers";
    for (auto it = m_info_gatherers.begin(); it != m_info_gatherers.end(); ++it)
      it->second.get()->stop_gathering_thread();
  } else {
    // find gatherer for suppled device name and stop it
    bool gatherer_found=false;
    for (auto it = m_info_gatherers.lower_bound(device_name); it != m_info_gatherers.end(); ++it) {
      TLOG_DEBUG(0) << get_name() << " Stopping info gatherer: " << it->first;
      it->second.get()->stop_gathering_thread();
      gatherer_found=true;
    } 
    if (!gatherer_found) ers::warning(AttemptedToControlNonExantInfoGatherer(ERS_HERE, "stop", device_name));
  }
}

// cmd stuff
void
TimingHardwareManager::process_hardware_commands(std::atomic<bool>& running_flag)
{

  std::ostringstream starting_stream;
  starting_stream << ": Starting process_hardware_commands() method.";
  TLOG_DEBUG(0) << get_name() << starting_stream.str();

  while (running_flag.load()) {
    timingcmd::TimingHwCmd timing_hw_cmd;

    try {
      m_hw_command_in_queue->pop(timing_hw_cmd, m_queue_timeout);
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // it is perfectly reasonable that there might be no commands in the queue
      // some fraction of the times that we check, so we just continue on and try again
      continue;
    }

    ++m_received_hw_commands_counter;

    TLOG_DEBUG(0) << get_name() << ": Received hardware command #" << m_received_hw_commands_counter.load()
                  << ", it is of type: " << timing_hw_cmd.id << ", targeting device: " << timing_hw_cmd.device;

    std::string hw_cmd_name = timing_hw_cmd.id;
    if (auto cmd = m_timing_hw_cmd_map_.find(hw_cmd_name); cmd != m_timing_hw_cmd_map_.end()) {

      ++m_accepted_hw_commands_counter;

      TLOG_DEBUG(0) << "Found hw cmd: " << hw_cmd_name;
      try {
        std::invoke(cmd->second, timing_hw_cmd);
      } catch (const std::exception& exception) {
        ers::error(FailedToExecuteHardwareCommand(ERS_HERE, hw_cmd_name, timing_hw_cmd.device, exception));
        ++m_failed_hw_commands_counter;
      }
    } else {
      ers::error(InvalidHardwareCommandID(ERS_HERE, hw_cmd_name));
      ++m_rejected_hw_commands_counter;
    }
  }

  std::ostringstream exiting_stream;
  exiting_stream << ": Exiting process_hardware_commands() method. Received " << m_received_hw_commands_counter.load()
                 << " commands";
  TLOG_DEBUG(0) << get_name() << exiting_stream.str();
}

// common commands
void
TimingHardwareManager::io_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::IOResetCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);
  
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " io reset";

  stop_hw_mon_gathering(hw_cmd.device);

  auto design_device = get_timing_device_plain(hw_cmd.device);
  auto design = dynamic_cast<const timing::TopDesignInterface*>(design_device);

  if (cmd_payload.soft) {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " soft io reset";
    design->soft_reset_io();
  } else {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                  << " io reset, with supplied clk file: " << cmd_payload.clock_config << "and fanout mode: " << cmd_payload.fanout_mode;
    design->reset_io(cmd_payload.fanout_mode, cmd_payload.clock_config);
  }
  start_hw_mon_gathering(hw_cmd.device);
}

void
TimingHardwareManager::print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " print status";

  auto design = dynamic_cast<const timing::TopDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  TLOG() << std::endl << design->get_status();
}

// master commands
void
TimingHardwareManager::set_timestamp(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " set timestamp";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->sync_timestamp();
}

// partition commands
void
TimingHardwareManager::partition_configure(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " configure";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);

  partition.reset();
  partition.configure(cmd_payload.trigger_mask, cmd_payload.spill_gate_enabled, cmd_payload.rate_control_enabled);
}

void
TimingHardwareManager::partition_enable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " enable";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.enable(true);
}

void
TimingHardwareManager::partition_disable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " disable";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.enable(false);
}

void
TimingHardwareManager::partition_start(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " start";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.start();
}

void
TimingHardwareManager::partition_stop(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " stop";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.stop();
}

void
TimingHardwareManager::partition_enable_triggers(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id
                << " start triggers";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.enable_triggers(true);
}

void
TimingHardwareManager::partition_disable_triggers(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " stop triggers";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  partition.enable_triggers(false);
}

void
TimingHardwareManager::partition_print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " print partition " << cmd_payload.partition_id << " status";

  auto design = dynamic_cast<const timing::MasterDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  auto partition = design->get_partition_node(cmd_payload.partition_id);
  TLOG() << std::endl << partition.get_status();
}

// endpoint commands
void
TimingHardwareManager::endpoint_enable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept enable, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;

  auto design = dynamic_cast<const timing::EndpointDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->enable(cmd_payload.partition, cmd_payload.address);
}

void
TimingHardwareManager::endpoint_disable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept disable";

  auto design = dynamic_cast<const timing::EndpointDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->disable();
}

void
TimingHardwareManager::endpoint_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept reset, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;

  auto design = dynamic_cast<const timing::EndpointDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->reset(cmd_payload.partition, cmd_payload.address);
}

void
TimingHardwareManager::hsi_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi reset";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_hsi_node().reset_hsi();
}

void
TimingHardwareManager::hsi_configure(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::HSIConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi configure";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->configure_hsi(
    cmd_payload.data_source, cmd_payload.rising_edge_mask, cmd_payload.falling_edge_mask, cmd_payload.invert_edge_mask, cmd_payload.random_rate);
}

void
TimingHardwareManager::hsi_start(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi start";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_hsi_node().start_hsi();
}

void
TimingHardwareManager::hsi_stop(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi stop";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  design->get_hsi_node().stop_hsi();
}

void
TimingHardwareManager::hsi_print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi print status";

  auto design = dynamic_cast<const timing::HSIDesignInterface*>(get_timing_device_plain(hw_cmd.device));
  TLOG() << std::endl << design->get_hsi_node().get_status();
}

} // namespace timinglibs
} // namespace dunedaq
