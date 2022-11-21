/**
 * @file TimestampEstimator.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "timinglibs/TimestampEstimator.hpp"
#include "timinglibs/TimingIssues.hpp"

#include "dfmessages/TimeSync.hpp"
#include "logging/Logging.hpp"

#include <memory>

#define TRACE_NAME "TimestampEstimator" // NOLINT

namespace dunedaq {
namespace timinglibs {
TimestampEstimator::TimestampEstimator(
  std::shared_ptr<iomanager::ReceiverConcept<dfmessages::TimeSync>> time_sync_receiver,
  uint64_t clock_frequency_hz) // NOLINT(build/unsigned)
  : m_clock_frequency_hz(clock_frequency_hz)
  , m_time_sync_receiver(time_sync_receiver)
  , m_estimator_thread(std::bind(&TimestampEstimator::estimator_thread_fn, this, std::placeholders::_1))
  , queueTimeout_(10)
{
  m_estimator_thread.start_working_thread("tde-ts-est");
}

TimestampEstimator::TimestampEstimator(uint64_t clock_frequency_hz) // NOLINT(build/unsigned)
  : m_clock_frequency_hz(clock_frequency_hz)
  , m_time_sync_receiver(nullptr)
  , m_estimator_thread(std::bind(&TimestampEstimator::estimator_thread_fn, this, std::placeholders::_1))
{
  m_most_recent_timesync.daq_time = dfmessages::TypeDefaults::s_invalid_timestamp;
  m_current_timestamp_estimate.store(dfmessages::TypeDefaults::s_invalid_timestamp);
}

TimestampEstimator::~TimestampEstimator()
{
  if (m_estimator_thread.thread_running()) {
    m_estimator_thread.stop_working_thread();
  }
}

void
TimestampEstimator::add_timestamp_datapoint(const dfmessages::TimeSync& ts)
{
  std::scoped_lock<std::mutex> lk(m_datapoint_mutex);

  // First, update the latest timestamp
  dfmessages::timestamp_t estimate = m_current_timestamp_estimate.load();
  dfmessages::timestamp_diff_t diff = estimate - ts.daq_time;
  TLOG_DEBUG(TLVL_TIME_SYNCS) << "Got a TimeSync timestamp = " << ts.daq_time << ", system time = " << ts.system_time
                 << " when current timestamp estimate was " << estimate << ". diff=" << diff << " run=" << ts.run_number
                 << " seqno=" << ts.sequence_number << " source_pid=" << ts.source_pid;
  if (m_most_recent_timesync.daq_time == dfmessages::TypeDefaults::s_invalid_timestamp ||
      ts.daq_time > m_most_recent_timesync.daq_time) {
    m_most_recent_timesync = ts;
  }

  if (m_most_recent_timesync.daq_time != dfmessages::TypeDefaults::s_invalid_timestamp) {
    // Update the current timestamp estimate, based on the most recently-read TimeSync
    using namespace std::chrono;
    // std::chrono is the worst
    auto time_now =
      static_cast<uint64_t>(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count()); // NOLINT

    // (PAR 2021-07-22) We only want to _increase_ our timestamp
    // estimate, not _decrease_ it, so we only attempt the update if
    // our system time is later than the latest time sync's system
    // time. We can get TimeSync messages from the "future" if
    // they're coming from another host whose clock is not exactly
    // synchronized with ours: that's fine, but if the discrepancy
    // is large, then badness could happen, so emit a warning

    if (time_now < m_most_recent_timesync.system_time - 10000) {
      ers::warning(EarlyTimeSync(ERS_HERE, m_most_recent_timesync.system_time - time_now));
    }

    if (time_now > m_most_recent_timesync.system_time) {

      auto delta_time = time_now - m_most_recent_timesync.system_time;
      TLOG_DEBUG(TLVL_TIME_SYNCS) << "Time diff between current system and latest TimeSync system time [us]: " << delta_time;

      // Warn user if current system time is more than 1s ahead of latest TimeSync system time. This could be a sign of
      // an issue, e.g. machine times out of sync
      if (delta_time > 1e6)
        ers::warning(LateTimeSync(ERS_HERE, delta_time));

      const dfmessages::timestamp_t new_timestamp =  m_most_recent_timesync.daq_time;
      TLOG() << "Skipped the addition of the delta_time to the most recent DAQ timestamp";

      //const dfmessages::timestamp_t new_timestamp =
      //  m_most_recent_timesync.daq_time + delta_time * m_clock_frequency_hz / 1000000;

      // Don't ever decrease the timestamp; just wait until enough
      // time passes that we want to increase it
      if (m_current_timestamp_estimate.load() == dfmessages::TypeDefaults::s_invalid_timestamp ||
          new_timestamp >= m_current_timestamp_estimate.load()) {
        m_current_timestamp_estimate.store(new_timestamp);
      } else {
        TLOG_DEBUG(TLVL_TIME_SYNCS) << "Not updating timestamp estimate backwards from " << m_current_timestamp_estimate.load()
                      << " to " << new_timestamp;
      }
    }
  }
}

void
TimestampEstimator::estimator_thread_fn(std::atomic<bool>& running_flag)
{
  // This loop is a hack to deal with the fact that there might be
  // leftover TimeSync messages from the previous run, because
  // ModuleLevelTrigger is stopped before the readout modules
  // that send the TimeSyncs. So we pop everything we can at
  // startup. This will definitely get all of the TimeSyncs from the
  // previous run. It *may* also get TimeSyncs from the current run,
  // which we drop on the floor. This is fairly harmless: it'll just
  // slightly delay us getting to the point where we actually start
  // making the timestamp estimate
  dfmessages::TimeSync tsync{ dfmessages::TypeDefaults::s_invalid_timestamp };
  while (true) {
    try {
      tsync = m_time_sync_receiver->receive(iomanager::Receiver::s_no_block);
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      break;
    }
  }

  m_most_recent_timesync.daq_time = dfmessages::TypeDefaults::s_invalid_timestamp;
  m_current_timestamp_estimate.store(dfmessages::TypeDefaults::s_invalid_timestamp);

  // m_time_sync_receiver is connected to an MPMC queue with multiple
  // writers. We read whatever we can off it, and the item with the
  // largest timestamp "wins"
  while (running_flag.load()) {
    // First, update the latest timestamp
    try {
      tsync = m_time_sync_receiver->receive(iomanager::Receiver::s_no_block);
      add_timestamp_datapoint(tsync);
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      // nothing received, we'll try again soon
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Drain the input queue as best we can. We're not going to do
  // anything with the TimeSync messages, so we just drop them on the
  // floor
  while (true) {
    try {
      tsync = m_time_sync_receiver->receive(iomanager::Receiver::s_no_block);
    } catch (const dunedaq::iomanager::TimeoutExpired& excpt) {
      break;
    }
  }
}

} // namespace timinglibs
} // namespace dunedaq
