/**
 * @file TimingController.cpp TimingController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingController.hpp"

#include "timing/timingcmd/Structs.hpp"
#include "timing/timingcmd/Nljs.hpp"

#include "TimingIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

namespace dunedaq {
namespace timing {

TimingController::TimingController(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_hw_command_out_queue(nullptr)
  , m_hw_cmd_out_queue_timeout(100)
{
}

void
TimingController::init( const nlohmann::json& init_data)
{
  // set up queues
  auto ini = init_data.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (!qi.name.compare("hardware_commands_out")) {
      try
      {
        m_hw_command_out_queue.reset(new sink_t(qi.inst));
      }
      catch (const ers::Issue& excpt)
      {
        throw InvalidQueueFatalError(ERS_HERE, get_name(), qi.name, excpt);
      }
    }
  }
}

void
TimingController::do_start(const nlohmann::json&)
{
}

void
TimingController::do_stop(const nlohmann::json&)
{
}

void
TimingController::send_hw_cmd(const timingcmd::TimingHwCmd& hw_cmd)
{
  std::string thisQueueName = m_hw_command_out_queue->get_name();
  try
  {
    m_hw_command_out_queue->push(hw_cmd, m_hw_cmd_out_queue_timeout);
  }
  catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
  {
    std::ostringstream oss_warn;
    oss_warn << "push to output queue \"" << thisQueueName << "\"";
    ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(),
      std::chrono::duration_cast<std::chrono::milliseconds>(m_hw_cmd_out_queue_timeout).count()));
  }
}

} // namespace timing 
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
