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
namespace timinglibs {

TimingController::TimingController(const std::string& name, uint number_hw_commands)
  : dunedaq::appfwk::DAQModule(name)
  , m_hw_command_out_queue_name("hardware_commands_out")
  , m_hw_command_out_queue(nullptr)
  , m_hw_cmd_out_queue_timeout(100)
  , m_timing_device("")
  , m_number_hw_commands(number_hw_commands)
  , m_sent_hw_command_counters(m_number_hw_commands)
{
  for (auto it = m_sent_hw_command_counters.begin(); it != m_sent_hw_command_counters.end(); ++it) {
    it->atomic.store(0);
  }
}

void
TimingController::init(const nlohmann::json& init_data)
{
  // set up queues
  auto qi = appfwk::connection_index(init_data, { m_hw_command_out_queue_name });
  iomanager::IOManager iom;
  m_hw_command_out_queue = iom.get_sender<timingcmd::TimingHwCmd>(qi[m_hw_command_out_queue_name]);
}

void
TimingController::do_start(const nlohmann::json&)
{
  // Timing commands are processed even before a start command. Counters may therefore lose counts after a start.
  // reset counters
  for (auto it = m_sent_hw_command_counters.begin(); it != m_sent_hw_command_counters.end(); ++it) {
    it->atomic.store(0);
  }
}

void
TimingController::do_stop(const nlohmann::json&)
{}

void
TimingController::send_hw_cmd(timingcmd::TimingHwCmd& hw_cmd)
{
  if (!m_hw_command_out_queue)
  {
    throw QueueIsNullFatalError(ERS_HERE, get_name(), m_hw_command_out_queue_name);
  }
  try {
    m_hw_command_out_queue->send(hw_cmd, m_hw_cmd_out_queue_timeout);
  } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
    std::ostringstream oss_warn;
    oss_warn << "push to output queue \"" << m_hw_command_out_queue_name << "\"";
    ers::warning(dunedaq::iomanager::TimeoutExpired(
      ERS_HERE,
      get_name(),
      oss_warn.str(),
      std::chrono::duration_cast<std::chrono::milliseconds>(m_hw_cmd_out_queue_timeout).count()));
  }
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
