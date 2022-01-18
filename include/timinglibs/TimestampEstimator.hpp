/**
 * @file TimestampEstimator.hpp TimestampEstimator Class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMESTAMPESTIMATOR_HPP_
#define TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMESTAMPESTIMATOR_HPP_

#include "timinglibs/TimestampEstimatorBase.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQSource.hpp"
#include "utilities/WorkerThread.hpp"

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/Types.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief TimestampEstimator is an implementation of
 * TimestampEstimatorBase that uses TimeSync messages from an input
 * queue to estimate the current timestamp
 **/
class TimestampEstimator : public TimestampEstimatorBase
{
public:
  TimestampEstimator(std::unique_ptr<appfwk::DAQSource<dfmessages::TimeSync>>& time_sync_source,
                     uint64_t clock_frequency_hz); // NOLINT(build/unsigned)

  explicit TimestampEstimator(uint64_t clock_frequency_hz); // NOLINT(build/unsigned)

  virtual ~TimestampEstimator();

  dfmessages::timestamp_t get_timestamp_estimate() const override { return m_current_timestamp_estimate.load(); }

  void add_timestamp_datapoint(const dfmessages::TimeSync& ts);

private:
  void estimator_thread_fn(std::atomic<bool>& running_flag);

  // The estimate of the current timestamp
  std::atomic<dfmessages::timestamp_t> m_current_timestamp_estimate{ dfmessages::TypeDefaults::s_invalid_timestamp };

  uint64_t m_clock_frequency_hz; // NOLINT(build/unsigned)
  appfwk::DAQSource<dfmessages::TimeSync>* m_time_sync_source;
  utilities::WorkerThread m_estimator_thread;
  dfmessages::TimeSync m_most_recent_timesync;
  std::mutex m_datapoint_mutex;
};

} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMESTAMPESTIMATOR_HPP_
