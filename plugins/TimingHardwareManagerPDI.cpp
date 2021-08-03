/**
 * @file TimingHardwareManager.cpp TimingHardwareManager class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingHardwareManagerPDI.hpp"

#include "timinglibs/timinghardwaremanagerpdi/Nljs.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Structs.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingHardwareManagerPDI::TimingHardwareManagerPDI(const std::string& name)
  : TimingHardwareManager(name)
{
  register_command("conf", &TimingHardwareManagerPDI::do_configure);
}

void
TimingHardwareManagerPDI::init(const nlohmann::json& init_data)
{
  TimingHardwareManager::init(init_data);

  // register only the commands which are needed for this hardware manager and each design

  // common
  register_common_hw_commands_for_design<timing::OverlordDesign<timing::TLUIONode>,
                                         timing::OverlordDesign<timing::FMCIONode>,

                                         timing::OuroborosDesign<timing::TLUIONode>,
                                         timing::OuroborosDesign<timing::FMCIONode>,

                                         timing::BoreasDesign<timing::FMCIONode>,
                                         timing::BoreasDesign<timing::TLUIONode>,

                                         timing::OuroborosMuxDesign<timing::PC059IONode>,
                                         timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>,

                                         timing::EndpointDesign<timing::FMCIONode>>();
  // master
  register_master_hw_commands_for_design<timing::OverlordDesign<timing::TLUIONode>,
                                         timing::OverlordDesign<timing::FMCIONode>,

                                         timing::BoreasDesign<timing::TLUIONode>,
                                         timing::BoreasDesign<timing::FMCIONode>,

                                         timing::OuroborosDesign<timing::TLUIONode>,
                                         timing::OuroborosDesign<timing::FMCIONode>,

                                         timing::OuroborosMuxDesign<timing::PC059IONode>,
                                         timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>();
  // endpoint
  register_endpoint_hw_commands_for_design<timing::OuroborosDesign<timing::TLUIONode>,
                                           timing::OuroborosDesign<timing::FMCIONode>,

                                           timing::BoreasDesign<timing::FMCIONode>,
                                           timing::BoreasDesign<timing::TLUIONode>,

                                           timing::OuroborosMuxDesign<timing::PC059IONode>,
                                           timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>,

                                           timing::EndpointDesign<timing::FMCIONode>,

                                           timing::ChronosDesign<timing::FMCIONode>>();
  // hsi
  register_hsi_hw_commands_for_design<timing::BoreasDesign<timing::FMCIONode>,
                                      timing::BoreasDesign<timing::TLUIONode>,

                                      timing::ChronosDesign<timing::FMCIONode>>();
}

template<class DSGN>
void
TimingHardwareManagerPDI::register_common_hw_commands_for_design()
{
  register_timing_hw_command("io_reset", typeid(DSGN).name(), &TimingHardwareManagerPDI::io_reset<DSGN>);
  register_timing_hw_command("print_status", typeid(DSGN).name(), &TimingHardwareManagerPDI::print_status<DSGN>);
}

template<class DSGN>
void
TimingHardwareManagerPDI::register_master_hw_commands_for_design()
{
  register_timing_hw_command("set_timestamp", typeid(DSGN).name(), &TimingHardwareManagerPDI::set_timestamp<DSGN>);
  register_timing_hw_command(
    "partition_configure", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_configure<DSGN>);
  register_timing_hw_command(
    "partition_enable", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_enable<DSGN>);
  register_timing_hw_command(
    "partition_disable", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_disable<DSGN>);
  register_timing_hw_command("partition_start", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_start<DSGN>);
  register_timing_hw_command("partition_stop", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_stop<DSGN>);
  register_timing_hw_command(
    "partition_enable_triggers", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_enable_triggers<DSGN>);
  register_timing_hw_command(
    "partition_disable_triggers", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_disable_triggers<DSGN>);
  register_timing_hw_command(
    "partition_print_status", typeid(DSGN).name(), &TimingHardwareManagerPDI::partition_print_status<DSGN>);
}

template<class DSGN>
void
TimingHardwareManagerPDI::register_endpoint_hw_commands_for_design()
{
  register_timing_hw_command("endpoint_enable", typeid(DSGN).name(), &TimingHardwareManagerPDI::endpoint_enable<DSGN>);
  register_timing_hw_command(
    "endpoint_disable", typeid(DSGN).name(), &TimingHardwareManagerPDI::endpoint_disable<DSGN>);
  register_timing_hw_command("endpoint_reset", typeid(DSGN).name(), &TimingHardwareManagerPDI::endpoint_reset<DSGN>);
}

template<class DSGN>
void
TimingHardwareManagerPDI::register_hsi_hw_commands_for_design()
{
  register_timing_hw_command("hsi_reset", typeid(DSGN).name(), &TimingHardwareManagerPDI::hsi_reset<DSGN>);
  register_timing_hw_command("hsi_configure", typeid(DSGN).name(), &TimingHardwareManagerPDI::hsi_configure<DSGN>);
  register_timing_hw_command("hsi_start", typeid(DSGN).name(), &TimingHardwareManagerPDI::hsi_start<DSGN>);
  register_timing_hw_command("hsi_stop", typeid(DSGN).name(), &TimingHardwareManagerPDI::hsi_stop<DSGN>);
  register_timing_hw_command(
    "hsi_print_status", typeid(DSGN).name(), &TimingHardwareManagerPDI::hsi_print_status<DSGN>);
}

void
TimingHardwareManagerPDI::do_configure(const nlohmann::json& obj)
{
  timinghardwaremanagerpdi::from_json(obj, m_cfg);

  m_connections_file = m_cfg.connections_file;

  TLOG() << get_name() << "conf: con. file before env var expansion: " << m_connections_file;
  resolve_environment_variables(m_connections_file);
  TLOG() << get_name() << "conf: con. file after env var expansion:  " << m_connections_file;

  if (!m_cfg.uhal_log_level.compare("debug")) {
    uhal::setLogLevelTo(uhal::Debug());
  } else if (!m_cfg.uhal_log_level.compare("info")) {
    uhal::setLogLevelTo(uhal::Info());
  } else if (!m_cfg.uhal_log_level.compare("notice")) {
    uhal::setLogLevelTo(uhal::Notice());
  } else if (!m_cfg.uhal_log_level.compare("warning")) {
    uhal::setLogLevelTo(uhal::Warning());
  } else if (!m_cfg.uhal_log_level.compare("error")) {
    uhal::setLogLevelTo(uhal::Error());
  } else if (!m_cfg.uhal_log_level.compare("fatal")) {
    uhal::setLogLevelTo(uhal::Fatal());
  } else {
    throw InvalidUHALLogLevel(ERS_HERE, m_cfg.uhal_log_level);
  }

  try {
    m_connection_manager = std::make_unique<uhal::ConnectionManager>("file://" + m_connections_file);
  } catch (const uhal::exception::FileNotFound& excpt) {
    std::stringstream message;
    message << m_connections_file << " not found. Has TIMING_SHARE been set?";
    throw UHALConnectionsFileIssue(ERS_HERE, message.str(), excpt);
  }

  // monitoring
  // only register monitor threads if we have been given the name of the device to monitor
  if (m_cfg.monitored_device_name_master.compare("")) {
    register_info_gatherer<timing::timingfirmwareinfo::OverlordTLUMonitorData,
                           timing::OverlordDesign<timing::TLUIONode>>(
      m_cfg.gather_interval, m_cfg.monitored_device_name_master, 1);
    
    register_info_gatherer<timing::timingfirmwareinfo::OverlordTLUMonitorDataDebug,
                           timing::OverlordDesign<timing::TLUIONode>>(
      m_cfg.gather_interval_debug, m_cfg.monitored_device_name_master, 2);

    register_info_gatherer<timing::timingfirmwareinfo::BoreasTLUMonitorData, timing::BoreasDesign<timing::TLUIONode>>(
      m_cfg.gather_interval, m_cfg.monitored_device_name_master, 1);
    
    register_info_gatherer<timing::timingfirmwareinfo::BoreasTLUMonitorDataDebug,
                           timing::BoreasDesign<timing::TLUIONode>>(
      m_cfg.gather_interval_debug, m_cfg.monitored_device_name_master, 2);

    register_info_gatherer<timing::timingfirmwareinfo::BoreasFMCMonitorData, timing::BoreasDesign<timing::FMCIONode>>(
      m_cfg.gather_interval, m_cfg.monitored_device_name_master, 1);
    
    register_info_gatherer<timing::timingfirmwareinfo::BoreasFMCMonitorDataDebug,
                           timing::BoreasDesign<timing::FMCIONode>>(
      m_cfg.gather_interval_debug, m_cfg.monitored_device_name_master, 2);

    register_info_gatherer<timing::timingfirmwareinfo::FanoutPC059MonitorData,
                             timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(
                                m_cfg.gather_interval, m_cfg.monitored_device_name_master, 1);
    
    register_info_gatherer<timing::timingfirmwareinfo::FanoutPC059MonitorDataDebug,
                            timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(
                              m_cfg.gather_interval_debug, m_cfg.monitored_device_name_master, 2);

    register_info_gatherer<timing::timingfirmwareinfo::OuroborosPC059MonitorData,
                             timing::OuroborosMuxDesign<timing::PC059IONode>>(
                                m_cfg.gather_interval, m_cfg.monitored_device_name_master, 1);
    
    register_info_gatherer<timing::timingfirmwareinfo::OuroborosPC059MonitorDataDebug,
                            timing::OuroborosMuxDesign<timing::PC059IONode>>(
                              m_cfg.gather_interval_debug, m_cfg.monitored_device_name_master, 2);
  }
  
  for (auto it = m_cfg.monitored_device_names_fanout.begin(); it != m_cfg.monitored_device_names_fanout.end(); ++it) {
    if (it->compare("")) {
      register_info_gatherer<timing::timingfirmwareinfo::FanoutPC059MonitorData,
                             timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(
                                m_cfg.gather_interval, *it, 1);
      register_info_gatherer<timing::timingfirmwareinfo::FanoutPC059MonitorDataDebug,
                            timing::FanoutDesign<timing::PC059IONode, timing::PDIMasterNode>>(
                              m_cfg.gather_interval_debug, *it, 2);
    }
  }
  
  if (m_cfg.monitored_device_name_endpoint.compare("")) {
    register_info_gatherer<timing::timingfirmwareinfo::TimingEndpointFMCMonitorData,
                           timing::EndpointDesign<timing::FMCIONode>>(
      m_cfg.gather_interval, m_cfg.monitored_device_name_endpoint, 1);
    register_info_gatherer<timing::timingfirmwareinfo::TimingEndpointFMCMonitorDataDebug,
                           timing::EndpointDesign<timing::FMCIONode>>(
      m_cfg.gather_interval_debug, m_cfg.monitored_device_name_endpoint, 2);
  }
  start_hw_mon_gathering();
  thread_.start_working_thread();
}

void
TimingHardwareManagerPDI::get_info(opmonlib::InfoCollector& ci, int level)
{

  // send counters internal to the module
  timinghardwaremanagerpdiinfo::Info module_info;
  module_info.received_hw_commands_counter = m_received_hw_commands_counter.load();
  module_info.accepted_hw_commands_counter = m_accepted_hw_commands_counter.load();
  module_info.rejected_hw_commands_counter = m_rejected_hw_commands_counter.load();
  module_info.failed_hw_commands_counter = m_failed_hw_commands_counter.load();
  
  ci.add(module_info);

  // the hardware device data
  timinghardwaremanagerpdiinfo::PDITimingHardwareData device_info;
  nlohmann::json device_data;
  timinghardwaremanagerpdiinfo::to_json(device_data, device_info);

  for (auto it = m_info_gatherers.begin(); it != m_info_gatherers.end(); ++it) {
    // master info
    if (m_cfg.monitored_device_name_master.find(it->second.get()->get_device_name()) != std::string::npos) {
      if (it->first.find("Debug") != std::string::npos) {
        it->second.get()->get_info(device_data["master_debug"], level);
      } else {
        it->second.get()->get_info(device_data["master"], level);
      }
    }
    
    for (uint i=0; i < m_cfg.monitored_device_names_fanout.size(); ++i) {
      std::string fanout_device_name = m_cfg.monitored_device_names_fanout.at(i);
      if (fanout_device_name.find(it->second.get()->get_device_name()) != std::string::npos) {
        if (it->first.find("Debug") != std::string::npos) {
          it->second.get()->get_info(device_data["fanout_"+std::to_string(i)+"_debug"], level);
        } else {
          it->second.get()->get_info(device_data["fanout_"+std::to_string(i)], level);
        }
      }
    }

    if (m_cfg.monitored_device_name_endpoint.find(it->second.get()->get_device_name()) != std::string::npos) {
      if (it->first.find("Debug") != std::string::npos) {
        it->second.get()->get_info(device_data["endpoint_debug"], level);
      } else {
        it->second.get()->get_info(device_data["endpoint"], level);
      }
    }
  }
  timinghardwaremanagerpdiinfo::from_json(device_data, device_info);
  
  ci.add(device_info);

  // maybe we should keep track of when we last send data, and only send if we have had an update since
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingHardwareManagerPDI)

// Local Variables:
// c-basic-offset: 2
// End:
