/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_SRC_HSIEVENTSENDER_HPP_
#define TIMINGLIBS_SRC_HSIEVENTSENDER_HPP_

#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModule.hpp"
#include "dfmessages/HSIEvent.hpp"
#include "utilities/WorkerThread.hpp"
#include <ers/Issue.hpp>

#include <bitset>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief HSIEventSender provides an interface to process and
 * and send HSIEvents.
 */
class HSIEventSender : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIEventSender Constructor
   * @param name Instance name for this HSIEventSender instance
   */
  explicit HSIEventSender(const std::string& name);

  HSIEventSender(const HSIEventSender&) = delete;            ///< HSIEventSender is not copy-constructible
  HSIEventSender& operator=(const HSIEventSender&) = delete; ///< HSIEventSender is not copy-assignable
  HSIEventSender(HSIEventSender&&) = delete;                 ///< HSIEventSender is not move-constructible
  HSIEventSender& operator=(HSIEventSender&&) = delete;      ///< HSIEventSender is not move-assignable

protected:
  // Commands
  virtual void do_configure(const nlohmann::json& obj) = 0;
  virtual void do_start(const nlohmann::json& obj) = 0;
  virtual void do_stop(const nlohmann::json& obj) = 0;
  virtual void do_scrap(const nlohmann::json& obj) = 0;

  // Threading
  virtual void do_hsievent_work(std::atomic<bool>&) = 0;
  dunedaq::utilities::WorkerThread m_thread;

  // Configuration
  std::string m_hsievent_send_connection;
  std::chrono::milliseconds m_queue_timeout;

  // push events to HSIEvent output queue
  virtual void send_hsi_event(dfmessages::HSIEvent& event, const std::string& location);
  virtual void send_hsi_event(dfmessages::HSIEvent& event) { send_hsi_event(event, m_hsievent_send_connection); }
  std::atomic<uint64_t> m_sent_counter;           // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_failed_to_send_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_sent_timestamp;    // NOLINT(build/unsigned)
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_HSIEVENTSENDER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
