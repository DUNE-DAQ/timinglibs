/**
 * @file TimestampEstimator.cpp
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "timinglibs/TimestampEstimator.hpp"
//#include "trigger/Issues.hpp"

#include "dfmessages/TimeSync.hpp"
#include "logging/Logging.hpp"

#include <memory>

#define TRACE_NAME "TimestampEstimator" // NOLINT

namespace dunedaq {
namespace timinglibs {
TimestampEstimator::TimestampEstimator(dfmessages::run_number_t run_number, uint64_t clock_frequency_hz) // NOLINT(build/unsigned)
  : m_clock_frequency_hz(clock_frequency_hz)
    , m_run_number(run_number)
{
    m_most_recent_timesync.daq_time = dfmessages::TypeDefaults::s_invalid_timestamp;
}

TimestampEstimator::TimestampEstimator(uint64_t clock_frequency_hz) // NOLINT(build/unsigned)
    : m_clock_frequency_hz(clock_frequency_hz)
{
    m_most_recent_timesync.daq_time = dfmessages::TypeDefaults::s_invalid_timestamp;
}

TimestampEstimator::~TimestampEstimator()
{
}

void
TimestampEstimator::add_timestamp_datapoint(const dfmessages::TimeSync& ts)
{
  std::scoped_lock<std::mutex> lk(m_datapoint_mutex);

  // First, update the latest timestamp
  dfmessages::timestamp_t estimate = m_current_timestamp_estimate.load();
  dfmessages::timestamp_diff_t diff = estimate - ts.daq_time;
  TLOG_DEBUG(10) << "Got a TimeSync timestamp = " << ts.daq_time << ", system time = " << ts.system_time
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
      TLOG_DEBUG(10) << "Time diff between current system and latest TimeSync system time [us]: " << delta_time;

      // Warn user if current system time is more than 1s ahead of latest TimeSync system time. This could be a sign of
      // an issue, e.g. machine times out of sync
      if (delta_time > 1e6)
        ers::warning(LateTimeSync(ERS_HERE, delta_time));

      const dfmessages::timestamp_t new_timestamp =
        m_most_recent_timesync.daq_time + delta_time * m_clock_frequency_hz / 1000000;
      // Don't ever decrease the timestamp; just wait until enough
      // time passes that we want to increase it
      if (m_current_timestamp_estimate.load() == dfmessages::TypeDefaults::s_invalid_timestamp ||
          new_timestamp >= m_current_timestamp_estimate.load()) {
        m_current_timestamp_estimate.store(new_timestamp);
      } else {
        TLOG_DEBUG(5) << "Not updating timestamp estimate backwards from " << m_current_timestamp_estimate.load()
                      << " to " << new_timestamp;
      }
    }
  }
}

void 
TimestampEstimator::timesync_callback(dfmessages::TimeSync& tsync)
{
    if (tsync.run_number == m_run_number) {
        add_timestamp_datapoint(tsync);
    }
}

} // namespace timinglibs
} // namespace dunedaq
