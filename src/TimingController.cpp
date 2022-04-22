/**
 * @file TimingController.cpp TimingController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingController.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingcmd/msgp.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "serialization/Serialization.hpp"
#include "ipm/Receiver.hpp"
#include "networkmanager/NetworkManager.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "ipm/Receiver.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingController::TimingController(const std::string& name, uint number_hw_commands)
  : dunedaq::appfwk::DAQModule(name)
  , m_hw_command_connection("")
  , m_hw_cmd_out_timeout(100)
  , m_timing_device("")
  , m_number_hw_commands(number_hw_commands)
  , m_sent_hw_command_counters(m_number_hw_commands)
  , m_device_info_connection("")
  , m_device_ready_timeout(10000)
  , m_device_ready(false)
  , m_device_infos_received_count(0)
{
  for (auto it = m_sent_hw_command_counters.begin(); it != m_sent_hw_command_counters.end(); ++it) {
    it->atomic.store(0);
  }
}

void
TimingController::do_configure(const nlohmann::json&)
{
  networkmanager::NetworkManager::get().subscribe(m_timing_device);
  networkmanager::NetworkManager::get().register_callback(
  m_timing_device, std::bind(&TimingController::process_device_info, this, std::placeholders::_1));
}

void
TimingController::do_start(const nlohmann::json&)
{
  // Timing commands are processed even before a start command. Counters may therefore lose counts after a start.
  // reset counters
  for (auto it = m_sent_hw_command_counters.begin(); it != m_sent_hw_command_counters.end(); ++it) {
    it->atomic.store(0);
  }
  m_device_infos_received_count=0;
}

void
TimingController::do_scrap(const nlohmann::json&)
{
  networkmanager::NetworkManager::get().clear_callback(m_timing_device);
  networkmanager::NetworkManager::get().unsubscribe(m_timing_device);
}

void
TimingController::send_hw_cmd(const timingcmd::TimingHwCmd& hw_cmd)
{
  bool was_successfully_sent = false;
  while (!was_successfully_sent) {
    try {
      auto serialised_cmd = dunedaq::serialization::serialize(hw_cmd, dunedaq::serialization::kMsgPack);

      networkmanager::NetworkManager::get().send_to(m_hw_command_connection,
                                    static_cast<const void*>(serialised_cmd.data()),
                                    serialised_cmd.size(),
                                    m_hw_cmd_out_timeout);
      was_successfully_sent = true;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      std::ostringstream oss_warn;
      oss_warn << "push to output connection \"" << m_hw_command_connection << "\"";
      ers::error(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_hw_cmd_out_timeout.count()));
    }
  }
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
