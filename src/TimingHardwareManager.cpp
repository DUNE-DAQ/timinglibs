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

#define TRACE_NAME "TimingHardwareManager" // NOLINT

namespace dunedaq {
namespace timinglibs {

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
    return dynamic_cast<const timing::TimingNode*> (&hw_device_entry->second->getNode(""));
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

    return dynamic_cast<const timing::TimingNode*> (&m_hw_device_map.find(device_name)->second->getNode(""));
  }
}

void
TimingHardwareManager::gather_monitor_data(InfoGathererInterface& gatherer)
{
  auto device_name = gatherer.get_device_name();

  
  while (gatherer.run_gathering()) {
    
    // collect the data from the hardware
    try {
      auto design = get_timing_device_plain(device_name);
      gatherer.collect_info_from_device(*design);
    } catch (const std::exception& excpt) {
      ers::error(FailedToCollectOpMonInfo(ERS_HERE, device_name, excpt));
    }

    auto prev_gather_time = std::chrono::steady_clock::now();
    auto next_gather_time = prev_gather_time + std::chrono::microseconds(gatherer.get_gather_interval());
    
    // check running_flag periodically
    auto slice_period = std::chrono::microseconds(1000);
    auto next_slice_gather_time = prev_gather_time + slice_period;
    
    bool break_flag = false;
    while (next_gather_time > next_slice_gather_time + slice_period) {
      if (!gatherer.run_gathering()) {
        TLOG() << "while waiting to gather data, negative run gatherer flag detected.";
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
  std::unique_ptr<InfoGathererInterface> gatherer = std::make_unique<InfoGathererInterface>(
    std::bind(&TimingHardwareManager::gather_monitor_data, this, std::placeholders::_1),
    gather_interval,
    device_name,
    op_mon_level);
  std::string gatherer_name = device_name + "_level_" + std::to_string(op_mon_level);
  TLOG_DEBUG(0) << "Registering info gatherer: " << gatherer_name;
  m_info_gatherers.emplace(std::make_pair(gatherer_name, std::move(gatherer)));
}

} // namespace timinglibs
} // namespace dunedaq
