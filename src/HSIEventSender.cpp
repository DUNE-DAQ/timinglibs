/**
 * @file HSIEventSender.cpp HSIEventSender class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIEventSender.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "iomanager/IOManager.hpp"
#include "appfwk/app/Nljs.hpp"
#include "serialization/Serialization.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

HSIEventSender::HSIEventSender(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&HSIEventSender::do_hsievent_work, this, std::placeholders::_1))
  , m_queue_timeout(1)
  , m_sent_counter(0)
  , m_failed_to_send_counter(0)
  , m_last_sent_timestamp(0)
{}

void
HSIEventSender::send_hsi_event(dfmessages::HSIEvent& event, const std::string& location)
{
  TLOG_DEBUG(3) << get_name() << ": Sending HSIEvent to " << location << ". \n" << event.header << ", " << std::bitset<32>(event.signal_map)
                << ", " << event.timestamp << ", " << event.sequence_counter << "\n";

  iomanager::IOManager iom;
  bool was_successfully_sent = false;
  while (!was_successfully_sent) {
    try {
      iom.get_sender<dfmessages::HSIEvent>(location)->send(event, m_queue_timeout);
      ++m_sent_counter;
      m_last_sent_timestamp.store(event.timestamp);
      was_successfully_sent = true;
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      std::ostringstream oss_warn;
      oss_warn << "push to output connection \"" << location << "\"";
      ers::error(dunedaq::iomanager::TimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
      ++m_failed_to_send_counter;
    }
  }
  if (m_sent_counter > 0 && m_sent_counter % 200000 == 0)
    TLOG_DEBUG(3) << "Have sent out " << m_sent_counter << " HSI events";
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
