/**
 * @file TimingHardwareManagerBase.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingHardwareManagerBase.hpp"

#include "timinglibs/dal/TimingHardwareManagerBase.hpp"

#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"
#include "timing/definitions.hpp"

#include "timing/FanoutDesign.hpp"
#include "timing/CDRMuxDesignInterface.hpp"
#include "timing/MasterMuxDesign.hpp"

#include "timing/timingfirmware/Nljs.hpp"
#include "timing/timingfirmware/Structs.hpp"
#include "appfwk/ModuleConfiguration.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#define TRACE_NAME "TimingHardwareManagerBase" // NOLINT

namespace dunedaq {

DUNE_DAQ_SERIALIZABLE(timinglibs::timingcmd::TimingHwCmd, "TimingHwCmd");
DUNE_DAQ_SERIALIZABLE(nlohmann::json, "JSON");

namespace timinglibs {

TimingHardwareManagerBase::TimingHardwareManagerBase(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_hw_cmd_connection("timing_cmds")
  , m_hw_command_receiver(nullptr)
  , m_gather_interval(1e6)
  , m_gather_interval_debug(10e6)
  , m_monitored_device_name_master("")
  , m_monitored_device_names_fanout({})
  , m_monitored_device_name_endpoint("")
  , m_monitored_device_name_hsi("")
  , m_received_hw_commands_counter{ 0 }
  , m_accepted_hw_commands_counter{ 0 }
  , m_rejected_hw_commands_counter{ 0 }
  , m_failed_hw_commands_counter{ 0 }
  , m_endpoint_scan_threads_clean_up_thread(nullptr)
{
  //  register_command("start", &TimingHardwareManagerBase::do_start);
  //  register_command("stop", &TimingHardwareManagerBase::do_stop);
   register_command("scrap", &TimingHardwareManagerBase::do_scrap);
}

void
TimingHardwareManagerBase::init(std::shared_ptr<appfwk::ModuleConfiguration> mcfg)
{
  auto mod_config = mcfg->module<timinglibs::dal::TimingHardwareManagerBase>(get_name());
  m_params = mod_config->get_configuration();

  // set up queues
  for (auto con : mod_config->get_inputs())
  {
    if (con->get_data_type() == datatype_to_string<timingcmd::TimingHwCmd>()) {
      m_hw_cmd_connection = con->UID();
      TLOG() << "m_hw_cmd_connection: " << m_hw_cmd_connection;
    }
  }

  try
  {
    m_hw_command_receiver = iomanager::IOManager::get()->get_receiver<timingcmd::TimingHwCmd>(m_hw_cmd_connection);
  } catch (const ers::Issue& excpt) {
    throw InvalidQueueFatalError(ERS_HERE, get_name(), "input", excpt);
  }

  m_endpoint_scan_threads_clean_up_thread = std::make_unique<dunedaq::utilities::ReusableThread>(0);
}

void
TimingHardwareManagerBase::conf(const nlohmann::json& data)
{
  m_received_hw_commands_counter = 0;
  m_accepted_hw_commands_counter = 0;
  m_rejected_hw_commands_counter = 0;
  m_failed_hw_commands_counter = 0;

  m_gather_interval = m_params->get_gather_interval();
  m_gather_interval_debug = m_params->get_gather_interval_debug();

  m_monitored_device_name_master = m_params->get_monitored_device_name_master();
  m_monitored_device_names_fanout = m_params->get_monitored_device_names_fanout();
  m_monitored_device_name_endpoint = m_params->get_monitored_device_name_endpoint();
  m_monitored_device_name_hsi = m_params->get_monitored_device_name_hsi();

  configure_uhal(m_params); // configure hw ipbus connection

  m_hw_command_receiver->add_callback(std::bind(&TimingHardwareManagerBase::process_hardware_command, this, std::placeholders::_1));

  m_run_endpoint_scan_cleanup_thread.store(true);
  m_endpoint_scan_threads_clean_up_thread->set_work(&TimingHardwareManagerBase::clean_endpoint_scan_threads, this);
}

void TimingHardwareManagerBase::do_scrap(const nlohmann::json& data)
{
  m_hw_command_receiver->remove_callback();

  auto time_of_scrap = std::chrono::high_resolution_clock::now();
  while(m_command_threads.size())
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms_since_scrap = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_of_scrap);
    TLOG_DEBUG(0) << "Have been waiting for " << ms_since_scrap.count() << " ms for " << m_command_threads.size() << " command threads to finish...";
    std::this_thread::sleep_for(std::chrono::microseconds(250000));
  }
  m_run_endpoint_scan_cleanup_thread.store(false);
  
  stop_hw_mon_gathering();
  
  scrap_uhal();

  m_command_threads.clear(); 
  m_info_gatherers.clear();
  m_timing_hw_cmd_map_.clear();
  m_hw_device_map.clear();
  m_connection_manager.reset();
}

const timing::TimingNode*
TimingHardwareManagerBase::get_timing_device_plain(const std::string& device_name)
{

  if (!device_name.compare("")) {
    std::stringstream message;
    message << "UHAL device name is an empty string";
    throw UHALDeviceNameIssue(ERS_HERE, message.str());
  }

  if (auto hw_device_entry = m_hw_device_map.find(device_name); hw_device_entry != m_hw_device_map.end()) {
    return dynamic_cast<const timing::TimingNode*>(&hw_device_entry->second->getNode(""));
  } else {
    TLOG_DEBUG(0) << get_name() << ": hw device interface for: " << device_name
                  << " does not exist. I will try to create it.";

    try {
      std::lock_guard<std::mutex> hw_device_map_guard(m_hw_device_map_mutex);
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
TimingHardwareManagerBase::gather_monitor_data(InfoGatherer& gatherer)
{
  auto device_name = gatherer.get_device_name();

  while (gatherer.run_gathering()) {

    // collect the data from the hardware
    try {
      auto design = get_timing_device<const timing::TopDesignInterface*>(device_name);

      gatherer.collect_info_from_device(*design);
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
TimingHardwareManagerBase::register_info_gatherer(uint gather_interval, const std::string& device_name, int op_mon_level)
{
  std::string gatherer_name = device_name + "_level_" + std::to_string(op_mon_level);
  if (m_info_gatherers.find(gatherer_name) == m_info_gatherers.end()) {
    std::unique_ptr<InfoGatherer> gatherer = std::make_unique<InfoGatherer>(
      std::bind(&TimingHardwareManagerBase::gather_monitor_data, this, std::placeholders::_1),
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
TimingHardwareManagerBase::start_hw_mon_gathering(const std::string& device_name)
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
TimingHardwareManagerBase::stop_hw_mon_gathering(const std::string& device_name)
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

std::vector<std::string>
TimingHardwareManagerBase::check_hw_mon_gatherer_is_running(const std::string& device_name)
{
  std::vector<std::string> running_gatherers;
  for (auto it = m_info_gatherers.lower_bound(device_name); it != m_info_gatherers.end(); ++it)
  {
    TLOG_DEBUG(0) << get_name() << " Checking run state of info gatherer: " << it->first << ", and the state is " << it->second.get()->run_gathering();
    if (it->second.get()->run_gathering())
    {
      running_gatherers.push_back(it->first);
    }
  }
  return running_gatherers;  
}

// cmd stuff

void
TimingHardwareManagerBase::process_hardware_command(timingcmd::TimingHwCmd& timing_hw_cmd)
{
  std::ostringstream starting_stream;
  starting_stream << ": Executing process_hardware_command() callback.";
  TLOG_DEBUG(0) << get_name() << starting_stream.str();

  ++m_received_hw_commands_counter;

  TLOG_DEBUG(0) << get_name() << ": Received hardware command #" << m_received_hw_commands_counter.load()
                  << ", it is of type: " << timing_hw_cmd.id << ", targeting device: " << timing_hw_cmd.device << ", with payload: " << timing_hw_cmd.payload.dump();

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

  std::ostringstream exiting_stream;
  exiting_stream << ": Finished executing process_hardware_command() callback. Received " << m_received_hw_commands_counter.load()
                 << " commands";
  TLOG_DEBUG(0) << get_name() << exiting_stream.str();
}

// common commands
void
TimingHardwareManagerBase::io_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::IOResetCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);
  
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " io reset";

  // io reset disrupts hw mon gathering, so stop if running
  auto running_hw_gatherers = check_hw_mon_gatherer_is_running(hw_cmd.device);
  for (auto& gatherer: running_hw_gatherers)
  {
    stop_hw_mon_gathering(gatherer);
  }

  auto design = get_timing_device<const timing::TopDesignInterface*>(hw_cmd.device);

  if (cmd_payload.soft) {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " soft io reset";
    design->soft_reset_io();
  } else if (!cmd_payload.clock_config.empty()) {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                  << " io reset, with supplied clk file: " << cmd_payload.clock_config;
    design->reset_io(cmd_payload.clock_config);
  }
  else
  {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                  << " io reset, with supplied clk source: " << cmd_payload.clock_source;
    design->reset_io(static_cast<timing::ClockSource>(cmd_payload.clock_source));
  }

  // if hw mon gathering was running previously, start it again
  for (auto& gatherer: running_hw_gatherers)
  {
    start_hw_mon_gathering(gatherer);
  }
}

void
TimingHardwareManagerBase::print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " print status";

  auto design = get_timing_device<const timing::TopDesignInterface*>(hw_cmd.device);
  TLOG() << std::endl << design->get_status();
}

// master commands
void
TimingHardwareManagerBase::set_timestamp(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::SyncTimestampPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                              << " set timestamp, with supplied ts source: " << cmd_payload.timestamp_source;

  auto design = get_timing_device<const timing::MasterDesignInterface*>(hw_cmd.device);
  design->sync_timestamp(static_cast<timing::TimestampSource>(cmd_payload.timestamp_source));
}

void
TimingHardwareManagerBase::master_endpoint_scan(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " master_endpoint_scan";

  std::stringstream command_thread_uid;
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  command_thread_uid << "enpoint_scan_cmd_at_" << std::put_time(&tm, "%d-%m-%Y %H-%M-%S") << "_cmd_num_" << m_accepted_hw_commands_counter.load();
  
  if (m_command_threads.size() > 5)
  {
    ers::warning(TooManyEndpointScanThreadsQueued(ERS_HERE, m_command_threads.size()));
  }
  else
  {
    TLOG_DEBUG(1) << "Queuing: " << command_thread_uid.str();

    auto thread_key = command_thread_uid.str();
    std::unique_lock map_lock(m_command_threads_map_mutex);

    m_command_threads.emplace(thread_key, std::make_unique<std::thread>(std::bind(&TimingHardwareManagerBase::perform_endpoint_scan, this, hw_cmd)));
  }
}

void TimingHardwareManagerBase::perform_endpoint_scan(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingMasterEndpointScanPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  for (auto& endpoint_location : cmd_payload.endpoints)
  {
    auto endpoint_address = endpoint_location.address;
    auto fanout_slot = endpoint_location.fanout_slot;
    auto sfp_slot = endpoint_location.sfp_slot;

    std::unique_lock<std::mutex> master_sfp_lock(master_sfp_mutex);

    TLOG_DEBUG(1) << get_name() << ": " << hw_cmd.device << " master_endpoint_scan starting: ept adr: " << endpoint_address << ", ept sfp: " << sfp_slot << ", fanout slot: " << fanout_slot;

    auto master_design = get_timing_device<const timing::MasterDesignInterface*>(hw_cmd.device);

    try
    {
      master_design->get_master_node_plain()->switch_endpoint_sfp(endpoint_address, true);

      if (sfp_slot >= 0)
      {
        if (fanout_slot > 0)
        {
          // configure fanout/FIB
          get_timing_device<const timing::CDRMuxDesignInterface*>(m_monitored_device_names_fanout.at(fanout_slot-1))->switch_cdr_mux(sfp_slot);

          // configure MIB
          dynamic_cast<const timing::CDRMuxDesignInterface*>(master_design)->switch_cdr_mux(fanout_slot-1);
        }
        else
        {
          dynamic_cast<const timing::MasterMuxDesign*>(master_design)->switch_downstream_mux_channel(sfp_slot, false);
        }
      }
      // configure any master mux, possibly
      master_design->get_master_node_plain()->scan_endpoint(endpoint_address, false);
      master_design->get_master_node_plain()->switch_endpoint_sfp(endpoint_address, false);
    }
    catch(std::exception& e)
    {
      ers::error(EndpointScanFailure(ERS_HERE,e));
      master_design->get_master_node_plain()->switch_endpoint_sfp(endpoint_address, false);
    }
  }
}

void TimingHardwareManagerBase::clean_endpoint_scan_threads()
{
  TLOG_DEBUG(0) << "Entering clean_endpoint_scan_threads()";
  bool break_flag = false;
  while (!break_flag)
  {
    for (auto& thread : m_command_threads)
    {
      if (thread.second->joinable())
      {
        std::unique_lock map_lock(m_command_threads_map_mutex);
        TLOG_DEBUG(2) << thread.first << " thread ready. Cleaning up.";
        thread.second->join();
        m_command_threads.erase(thread.first);
      }
    }
    
    auto prev_clean_time = std::chrono::steady_clock::now();
    auto next_clean_time = prev_clean_time + std::chrono::milliseconds(30);

    // check running_flag periodically
    auto flag_check_period = std::chrono::milliseconds(1);
    auto next_flag_check_time = prev_clean_time + flag_check_period;

    while (next_clean_time > next_flag_check_time + flag_check_period) {
      if (!m_run_endpoint_scan_cleanup_thread.load()) {
        TLOG_DEBUG(2) << "while waiting to clean up endpoint scan threads, negative run gatherer flag detected.";
        break_flag = true;
        break;
      }
      std::this_thread::sleep_until(next_flag_check_time);
      next_flag_check_time = next_flag_check_time + flag_check_period;
    }
    if (break_flag == false) {
      std::this_thread::sleep_until(next_clean_time);
    }
  }
  TLOG_DEBUG(0) << "Exiting clean_endpoint_scan_threads()";
}

// master commands
void
TimingHardwareManagerBase::set_endpoint_delay(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " set endpoint delay";
  
  timingcmd::TimingMasterSetEndpointDelayCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<const timing::MasterDesignInterface*>(hw_cmd.device);
  design->apply_endpoint_delay(cmd_payload.address, cmd_payload.coarse_delay, cmd_payload.fine_delay, cmd_payload.phase_delay, cmd_payload.measure_rtt, cmd_payload.control_sfp, cmd_payload.sfp_mux);
}

void
TimingHardwareManagerBase::send_fl_cmd(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingMasterSendFLCmdCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " send fl cmd. Payload: " << hw_cmd.payload.dump()
         << ", parsed data: " << cmd_payload.fl_cmd_id
         << ", " << cmd_payload.channel
         << ", " << cmd_payload.number_of_commands_to_send;

  auto design = get_timing_device<const timing::MasterDesignInterface*>(hw_cmd.device);
  design->get_master_node_plain()->send_fl_cmd(cmd_payload.fl_cmd_id, cmd_payload.channel, cmd_payload.number_of_commands_to_send);
}

// endpoint commands
void
TimingHardwareManagerBase::endpoint_enable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept enable, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;

  auto design = get_timing_device<const timing::EndpointDesignInterface*>(hw_cmd.device);
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->enable(cmd_payload.address, cmd_payload.partition);
}

void
TimingHardwareManagerBase::endpoint_disable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept disable";

  auto design = get_timing_device<const timing::EndpointDesignInterface*>(hw_cmd.device);
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->disable();
}

void
TimingHardwareManagerBase::endpoint_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept reset, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;

  auto design = get_timing_device<const timing::EndpointDesignInterface*>(hw_cmd.device);
  design->get_endpoint_node_plain(cmd_payload.endpoint_id)->reset(cmd_payload.address, cmd_payload.partition);
}

// hsi commands
void
TimingHardwareManagerBase::hsi_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi reset";

  auto design = get_timing_device<const timing::HSIDesignInterface*>(hw_cmd.device);
  design->get_hsi_node().reset_hsi();
}

void
TimingHardwareManagerBase::hsi_configure(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::HSIConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi configure";

  auto design = get_timing_device<const timing::HSIDesignInterface*>(hw_cmd.device);
  design->configure_hsi(
    cmd_payload.data_source, cmd_payload.rising_edge_mask, cmd_payload.falling_edge_mask, cmd_payload.invert_edge_mask, cmd_payload.random_rate);
}

void
TimingHardwareManagerBase::hsi_start(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi start";

  auto design = get_timing_device<const timing::HSIDesignInterface*>(hw_cmd.device);
  design->get_hsi_node().start_hsi();
}

void
TimingHardwareManagerBase::hsi_stop(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi stop";

  auto design = get_timing_device<const timing::HSIDesignInterface*>(hw_cmd.device);
  design->get_hsi_node().stop_hsi();
}

void
TimingHardwareManagerBase::hsi_print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi print status";

  auto design = get_timing_device<const timing::HSIDesignInterface*>(hw_cmd.device);
  TLOG() << std::endl << design->get_hsi_node().get_status();
}

} // namespace timinglibs
} // namespace dunedaq
