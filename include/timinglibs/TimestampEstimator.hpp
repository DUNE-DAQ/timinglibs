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

#include "dfmessages/TimeSync.hpp"
#include "dfmessages/Types.hpp"
#include "iomanager/Receiver.hpp"
#include "utilities/WorkerThread.hpp"

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

  TimestampEstimator(dfmessages::run_number_t run_number, uint64_t clock_frequency_hz); // NOLINT(build/unsigned)
  
  explicit TimestampEstimator(uint64_t clock_frequency_hz); // NOLINT(build/unsigned)

  virtual ~TimestampEstimator();

  dfmessages::timestamp_t get_timestamp_estimate() const override { return m_current_timestamp_estimate.load(); }

  void add_timestamp_datapoint(const dfmessages::TimeSync& ts);

  void timesync_callback(dfmessages::TimeSync& tsync);

private:

  // The estimate of the current timestamp
  std::atomic<dfmessages::timestamp_t> m_current_timestamp_estimate{ dfmessages::TypeDefaults::s_invalid_timestamp };

  uint64_t m_clock_frequency_hz; // NOLINT(build/unsigned)
  dfmessages::TimeSync m_most_recent_timesync;
  std::mutex m_datapoint_mutex;
  dfmessages::run_number_t m_run_number {0};
};

} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMESTAMPESTIMATOR_HPP_
