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

#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "ers/Issue.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {

DUNE_DAQ_SERIALIZABLE(timinglibs::timingcmd::TimingHwCmd);
DUNE_DAQ_SERIALIZABLE(nlohmann::json);

namespace timinglibs {

TimingController::TimingController(const std::string& name, uint number_hw_commands)
  : dunedaq::appfwk::DAQModule(name)
  , m_hw_command_out_connection("timing_cmds")
  , m_hw_cmd_out_timeout(100)
  , m_hw_command_sender(nullptr)
  , m_timing_device("")
  , m_device_info_receiver(nullptr)
  , m_number_hw_commands(number_hw_commands)
  , m_sent_hw_command_counters(m_number_hw_commands)
  , m_device_ready_timeout(10000)
  , m_device_ready(false)
  , m_device_infos_received_count(0)
{
  for (auto it = m_sent_hw_command_counters.begin(); it != m_sent_hw_command_counters.end(); ++it) {
    it->atomic.store(0);
  }
}

void
TimingController::init(const nlohmann::json& init_data)
{
  // set up queues
  auto qi = appfwk::connection_index(init_data, { m_hw_command_out_connection });
  m_hw_command_sender = get_iom_sender<timingcmd::TimingHwCmd>(qi[m_hw_command_out_connection]);
}

void
TimingController::do_configure(const nlohmann::json&)
{
  m_device_info_receiver = get_iom_receiver<nlohmann::json>(m_timing_device);
  m_device_info_receiver->add_callback(std::bind(&TimingController::process_device_info, this, std::placeholders::_1));
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
  m_device_info_receiver->remove_callback();
}

void
TimingController::send_hw_cmd(timingcmd::TimingHwCmd&& hw_cmd)
{
  if (!m_hw_command_sender)
  {
    throw QueueIsNullFatalError(ERS_HERE, get_name(), m_hw_command_out_connection);
  }
  try {
    m_hw_command_sender->send(std::move(hw_cmd), m_hw_cmd_out_timeout);
  } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
    std::ostringstream oss_warn;
    oss_warn << "push to output queue \"" << m_hw_command_out_connection << "\"";
    ers::warning(dunedaq::iomanager::TimeoutExpired(
      ERS_HERE,
      get_name(),
      oss_warn.str(),
      std::chrono::duration_cast<std::chrono::milliseconds>(m_hw_cmd_out_timeout).count()));
  }
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
