namespace dunedaq::timinglibs {

TimingHardwareManager::TimingHardwareManager(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , thread_(std::bind(&TimingHardwareManager::process_hardware_commands, this, std::placeholders::_1))
  , m_hw_command_in_queue(nullptr)
  , m_queue_timeout(100)
  , m_connection_manager(nullptr)
  , m_received_hw_commands_counter{ 0 }
  , m_accepted_hw_commands_counter{ 0 }
  , m_rejected_hw_commands_counter{ 0 }
  , m_failed_hw_commands_counter{ 0 }
{
  // all hardware manager variants will need these commands
  register_command("start", &TimingHardwareManager::do_start);
  register_command("stop", &TimingHardwareManager::do_stop);
  register_command("scrap", &TimingHardwareManager::do_scrap);
}

void
TimingHardwareManager::init(const nlohmann::json& init_data)
{
  // set up queues
  auto ini = init_data.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (!qi.name.compare("hardware_commands_in")) {
      try {
        m_hw_command_in_queue.reset(new source_t(qi.inst));
      } catch (const ers::Issue& excpt) {
        throw InvalidQueueFatalError(ERS_HERE, get_name(), qi.name, excpt);
      }
    }
  }
}

void
TimingHardwareManager::do_start(const nlohmann::json&)
{
  m_received_hw_commands_counter = 0;
  m_accepted_hw_commands_counter = 0;
  m_rejected_hw_commands_counter = 0;
  m_failed_hw_commands_counter = 0;
  TLOG() << get_name() << " successfully started";
}

void
TimingHardwareManager::do_stop(const nlohmann::json&)
{
  TLOG() << get_name() << " successfully stopped";
}

void
TimingHardwareManager::do_scrap(const nlohmann::json&)
{
  // TODO other scraping stuff
  thread_.stop_working_thread();
  stop_hw_mon_gathering();
  m_received_hw_commands_counter = 0;
  m_accepted_hw_commands_counter = 0;
  m_rejected_hw_commands_counter = 0;
  m_failed_hw_commands_counter = 0;
}

template<class INFO, class DSGN>
void
TimingHardwareManager::register_info_gatherer(uint gather_interval, const std::string& device_name, int op_mon_level)
{

  try {
    if (typeid(const DSGN&) != typeid(m_connection_manager->getDevice(device_name).getNode(""))) {
      TLOG_DEBUG(0) << device_name << " is not of type " << typeid(DSGN).name() << ". I will not monitor the hw";
      return;
    }
  } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
    std::stringstream message;
    message << "UHAL device name not " << device_name << " in connections file";
    throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
  }

  std::unique_ptr<InfoGathererInterface> gatherer = std::make_unique<InfoGatherer<INFO>>(
    std::bind(&TimingHardwareManager::gather_monitor_data<INFO, DSGN>, this, std::placeholders::_1),
    gather_interval,
    device_name,
    op_mon_level);
  std::string gatherer_name = device_name + "_" + typeid(DSGN).name() + "_" + typeid(INFO).name();
  TLOG_DEBUG(0) << "Registering info gatherer: " << gatherer_name;
  m_info_gatherers.emplace(std::make_pair(gatherer_name, std::move(gatherer)));
}

template<class INFO, class DSGN>
void
TimingHardwareManager::gather_monitor_data(InfoGatherer<INFO>& gatherer)
{
  auto device_name = gatherer.get_device_name();

  while (gatherer.run_gathering()) {
    // monitoring data recepticle
    INFO mon_data;

    // collect the data from the hardware
    try {
      auto design = get_timing_device<DSGN>(device_name);
      design.get_info(mon_data);

      // when did we actually collect the data
      mon_data.time_gathered = static_cast<int64_t>(std::time(nullptr));

      gatherer.update_last_gathered_time(mon_data.time_gathered);

      // store the monitor data for retrieveal by get_info at a later time
      gatherer.update_monitoring_data(mon_data);

      // sleep for a bit
      usleep(gatherer.get_gather_interval());
    } catch (const std::exception& excpt) {
      ers::error(FailedToCollectOpMonInfo(ERS_HERE, mon_data.info_type, device_name, excpt));
    }
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
    if (!gatherer_found) ers::error(AttemptedToControlNonExantInfoGatherer(ERS_HERE, "start", device_name));
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
    if (!gatherer_found) ers::error(AttemptedToControlNonExantInfoGatherer(ERS_HERE, "stop", device_name));
  }
}

// cmd stuff

void
TimingHardwareManager::construct_hw_cmd_name(const timingcmd::TimingHwCmd& hw_cmd, std::string& hw_cmd_name)
{
  hw_cmd_name = hw_cmd.id + "_" + typeid(m_connection_manager->getDevice(hw_cmd.device).getNode()).name();
  TLOG_DEBUG(0) << get_name() << ": constructed hw cmd name: " << hw_cmd_name;
}

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

    std::string hw_cmd_name;
    try {
      construct_hw_cmd_name(timing_hw_cmd, hw_cmd_name);
    } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
      std::stringstream message;
      message << "UHAL device name not " << timing_hw_cmd.device << " in connections file";
      throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
    }

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

template<class Child>
void
TimingHardwareManager::register_timing_hw_command(const std::string& hw_cmd_id,
                                                  const std::string& design_type,
                                                  void (Child::*f)(const timingcmd::TimingHwCmd&))
{
  using namespace std::placeholders;

  std::string hw_cmd_name = hw_cmd_id + "_" + design_type;
  TLOG_DEBUG(0) << "Registering timing hw command id: " << hw_cmd_name << " called with " << typeid(f).name()
                << std::endl;

  bool done = m_timing_hw_cmd_map_.emplace(hw_cmd_name, std::bind(f, dynamic_cast<Child*>(this), _1)).second;
  if (!done) {
    throw TimingHardwareCommandRegistrationFailed(ERS_HERE, hw_cmd_name, get_name());
  }
}

template<class TIMING_DEV>
const TIMING_DEV&
TimingHardwareManager::get_timing_device(const std::string& device_name)
{

  if (!device_name.compare("")) {
    std::stringstream message;
    message << "UHAL device name is an empty string";
    throw UHALDeviceNameIssue(ERS_HERE, message.str());
  }

  std::lock_guard<std::mutex> hw_device_map_guard(m_hw_device_map_mutex);

  if (auto hw_device_entry = m_hw_device_map.find(device_name); hw_device_entry != m_hw_device_map.end()) {
    return hw_device_entry->second->getNode<TIMING_DEV>("");
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

    return m_hw_device_map.find(device_name)->second->getNode<TIMING_DEV>("");
  }
}

// common commands
template<class DSGN>
void
TimingHardwareManager::io_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::IOResetCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);
  
  stop_hw_mon_gathering(hw_cmd.device);
  auto design = get_timing_device<DSGN>(hw_cmd.device);

  if (cmd_payload.soft) {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " soft io reset";
    design.get_io_node().soft_reset();
  } else {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                  << " io reset, with supplied clk file: " << cmd_payload.clock_config;
    design.get_io_node().reset(cmd_payload.clock_config);
  }
  start_hw_mon_gathering(hw_cmd.device);
}

template<>
void
TimingHardwareManager::io_reset<timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::IOResetCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);
  
  stop_hw_mon_gathering(hw_cmd.device);
  auto design = get_timing_device<timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(hw_cmd.device);

  if (cmd_payload.soft) {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " soft io reset";
    design.get_io_node().soft_reset();
  } else {
    TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device
                  << " io reset, with fanout mode: " << cmd_payload.fanout_mode << ", and supplied clk file: " << cmd_payload.clock_config;
    design.reset(cmd_payload.fanout_mode, cmd_payload.clock_config);
  }
  start_hw_mon_gathering(hw_cmd.device);
}

template<class DSGN>
void
TimingHardwareManager::print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto design = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " print status";
  TLOG() << std::endl << design.get_status();
}

// master commands
template<class DSGN>
void
TimingHardwareManager::set_timestamp(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto design = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " set timestamp";
  design.sync_timestamp();
}

// partition commands
template<class DSGN>
void
TimingHardwareManager::partition_configure(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " configure";

  partition.reset();
  partition.configure(cmd_payload.trigger_mask, cmd_payload.spill_gate_enabled, cmd_payload.rate_control_enabled);
}

template<class DSGN>
void
TimingHardwareManager::partition_enable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " enable";
  partition.enable(true);
}

template<class DSGN>
void
TimingHardwareManager::partition_disable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " disable";
  partition.enable(false);
}

template<class DSGN>
void
TimingHardwareManager::partition_start(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " start";
  partition.start();
}

template<class DSGN>
void
TimingHardwareManager::partition_stop(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " stop";
  partition.stop();
}

template<class DSGN>
void
TimingHardwareManager::partition_enable_triggers(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id
                << " start triggers";
  partition.enable_triggers(true);
}

template<class DSGN>
void
TimingHardwareManager::partition_disable_triggers(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " partition " << cmd_payload.partition_id << " stop triggers";
  partition.enable_triggers(false);
}

template<class DSGN>
void
TimingHardwareManager::partition_print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingPartitionCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto design = get_timing_device<DSGN>(hw_cmd.device);
  auto partition = design.get_master_node().get_partition_node(cmd_payload.partition_id);

  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " print partition " << cmd_payload.partition_id << " status";
  TLOG() << std::endl << partition.get_status();
}

// endpoint commands
template<class DSGN>
void
TimingHardwareManager::endpoint_enable(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept enable, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;
  desgin.get_endpoint_node(0).enable(cmd_payload.partition, cmd_payload.address);
}

template<class DSGN>
void
TimingHardwareManager::endpoint_disable(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept disable";
  desgin.get_endpoint_node(0).disable();
}

template<class DSGN>
void
TimingHardwareManager::endpoint_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::TimingEndpointConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " ept reset, adr: " << cmd_payload.address
                << ", part: " << cmd_payload.partition;
  desgin.get_endpoint_node(0).reset(cmd_payload.partition, cmd_payload.address);
}

template<class DSGN>
void
TimingHardwareManager::hsi_reset(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi reset";
  desgin.get_hsi_node().reset_hsi();
}

template<class DSGN>
void
TimingHardwareManager::hsi_configure(const timingcmd::TimingHwCmd& hw_cmd)
{
  timingcmd::HSIConfigureCmdPayload cmd_payload;
  timingcmd::from_json(hw_cmd.payload, cmd_payload);

  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi configure";

  desgin.get_hsi_node().configure_hsi(
    cmd_payload.data_source, cmd_payload.rising_edge_mask, cmd_payload.falling_edge_mask, cmd_payload.invert_edge_mask);
}

template<class DSGN>
void
TimingHardwareManager::hsi_start(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi start";
  desgin.get_hsi_node().start_hsi();
}

template<class DSGN>
void
TimingHardwareManager::hsi_stop(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi stop";
  desgin.get_hsi_node().stop_hsi();
}

template<class DSGN>
void
TimingHardwareManager::hsi_print_status(const timingcmd::TimingHwCmd& hw_cmd)
{
  auto desgin = get_timing_device<DSGN>(hw_cmd.device);
  TLOG_DEBUG(0) << get_name() << ": " << hw_cmd.device << " hsi print status";
  desgin.get_hsi_node().get_status();
}

} // namespace dunedaq::timinglibs
