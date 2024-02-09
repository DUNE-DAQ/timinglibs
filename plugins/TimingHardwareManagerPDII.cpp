/**
 * @file TimingHardwareManager.cpp TimingHardwareManager class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingHardwareManagerPDII.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Nljs.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/TimingIssues.hpp"

#include "timing/PDIMasterNode.hpp"
#include "timing/MasterNode.hpp"

#include "appfwk/DAQModuleHelper.hpp"
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

TimingHardwareManagerPDII::TimingHardwareManagerPDII(const std::string& name)
  : TimingHardwareManager(name)
{
  register_command("conf", &TimingHardwareManagerPDII::conf);
  register_command("start", &TimingHardwareManagerPDII::start);
  register_command("stop", &TimingHardwareManagerPDII::stop);
  register_command("scrap", &TimingHardwareManagerPDII::scrap);

}

void
TimingHardwareManagerPDII::conf(const nlohmann::json& conf_data)
{
  register_common_hw_commands_for_design();
  register_master_hw_commands_for_design();
  register_endpoint_hw_commands_for_design();
  register_hsi_hw_commands_for_design();

  auto conf_params = conf_data.get<timinghardwaremanagerpdi::ConfParams>();

  m_gather_interval = conf_params.gather_interval;
  m_gather_interval_debug = conf_params.gather_interval_debug;

  m_monitored_device_name_master = conf_params.monitored_device_name_master;
  m_monitored_device_names_fanout = conf_params.monitored_device_names_fanout;
  m_monitored_device_name_endpoint = conf_params.monitored_device_name_endpoint;
  m_monitored_device_name_hsi = conf_params.monitored_device_name_hsi;

  TimingHardwareManager::conf(conf_data);

  // monitoring
  // only register monitor threads if we have been given the name of the device to monitor
  if (m_monitored_device_name_master.compare("")) {
    register_info_gatherer(m_gather_interval, m_monitored_device_name_master, 1);
    //register_info_gatherer(m_gather_interval_debug, m_monitored_device_name_master, 2);
  }

  for (auto it = m_monitored_device_names_fanout.begin(); it != m_monitored_device_names_fanout.end(); ++it) {
    if (it->compare("")) {
      register_info_gatherer(m_gather_interval, *it, 1);
      //register_info_gatherer(m_gather_interval_debug, *it, 2);
    }
  }

  if (m_monitored_device_name_endpoint.compare("")) {
    register_info_gatherer(m_gather_interval, m_monitored_device_name_endpoint, 1);
    //register_info_gatherer(m_gather_interval_debug, m_monitored_device_name_endpoint, 2);
  }

  if (m_monitored_device_name_hsi.compare("")) {
    register_info_gatherer(m_gather_interval, m_monitored_device_name_hsi, 1);
    //register_info_gatherer(m_gather_interval_debug, m_monitored_device_name_hsi, 2);
  }

  start_hw_mon_gathering();
} // NOLINT

void
TimingHardwareManagerPDII::start(const nlohmann::json& /*data*/)
{
}
void
TimingHardwareManagerPDII::stop(const nlohmann::json& /*data*/)
{
}

void
TimingHardwareManagerPDII::register_common_hw_commands_for_design()
{
  register_timing_hw_command("io_reset", &TimingHardwareManagerPDII::io_reset);
  register_timing_hw_command("print_status", &TimingHardwareManagerPDII::print_status);
}

void
TimingHardwareManagerPDII::register_master_hw_commands_for_design()
{
  register_timing_hw_command("set_timestamp", &TimingHardwareManagerPDII::set_timestamp);
  register_timing_hw_command("set_endpoint_delay", &TimingHardwareManagerPDII::set_endpoint_delay);
  register_timing_hw_command("send_fl_command", &TimingHardwareManagerPDII::send_fl_cmd);
  register_timing_hw_command("master_endpoint_scan", &TimingHardwareManagerPDII::master_endpoint_scan);
}

void
TimingHardwareManagerPDII::register_endpoint_hw_commands_for_design()
{
  register_timing_hw_command("endpoint_enable", &TimingHardwareManagerPDII::endpoint_enable);
  register_timing_hw_command("endpoint_disable", &TimingHardwareManagerPDII::endpoint_disable);
  register_timing_hw_command("endpoint_reset", &TimingHardwareManagerPDII::endpoint_reset);
}

void
TimingHardwareManagerPDII::register_hsi_hw_commands_for_design()
{
  register_timing_hw_command("hsi_reset", &TimingHardwareManagerPDII::hsi_reset);
  register_timing_hw_command("hsi_configure", &TimingHardwareManagerPDII::hsi_configure);
  register_timing_hw_command("hsi_start", &TimingHardwareManagerPDII::hsi_start);
  register_timing_hw_command("hsi_stop", &TimingHardwareManagerPDII::hsi_stop);
  register_timing_hw_command("hsi_print_status", &TimingHardwareManagerPDII::hsi_print_status);
}

void
TimingHardwareManagerPDII::get_info(opmonlib::InfoCollector& ci, int level)
{

  // send counters internal to the module
  timinghardwaremanagerpdiinfo::Info module_info;
  module_info.received_hw_commands_counter = m_received_hw_commands_counter.load();
  module_info.accepted_hw_commands_counter = m_accepted_hw_commands_counter.load();
  module_info.rejected_hw_commands_counter = m_rejected_hw_commands_counter.load();
  module_info.failed_hw_commands_counter = m_failed_hw_commands_counter.load();

  ci.add(module_info);

  // the hardware device data
  for (auto it = m_info_gatherers.begin(); it != m_info_gatherers.end(); ++it) {
    // master info
    if (m_monitored_device_name_master.find(it->second.get()->get_device_name()) != std::string::npos) {
      if (it->second.get()->get_op_mon_level() <= level) {
        it->second.get()->add_info_to_collector("master", ci);
      }
    }

    for (uint i = 0; i < m_monitored_device_names_fanout.size(); ++i) {
      std::string fanout_device_name = m_monitored_device_names_fanout.at(i);
      if (fanout_device_name.find(it->second.get()->get_device_name()) != std::string::npos) {
        if (it->second.get()->get_op_mon_level() <= level) {
          it->second.get()->add_info_to_collector("fanout_" + std::to_string(i), ci);
        }
      }
    }

    if (m_monitored_device_name_endpoint.find(it->second.get()->get_device_name()) != std::string::npos) {
      if (it->second.get()->get_op_mon_level() <= level) {
        it->second.get()->add_info_to_collector("endpoint", ci);
      }
    }

    if (m_monitored_device_name_hsi.find(it->second.get()->get_device_name()) != std::string::npos) {
      if (it->second.get()->get_op_mon_level() <= level) {
        it->second.get()->add_info_to_collector("hsi", ci);
      }
    }
  }
}

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingHardwareManagerPDII)

// Local Variables:
// c-basic-offset: 2
// End: