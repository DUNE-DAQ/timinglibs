/**
 * @file TimestampEstimatorSystem_test.cxx  TimestampEstimatorSystem class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "iomanager/IOManager.hpp"
#include "iomanager/Sender.hpp"
#include "iomanager/Receiver.hpp"
#include "timinglibs/TimestampEstimator.hpp"

/**
 * @brief Name of this test module
 */
#define BOOST_TEST_MODULE TimestampEstimator_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>

using namespace dunedaq;

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

/**
 * @brief Initializes the QueueRegistry
 */
struct DAQSinkDAQSourceTestFixture
{
  DAQSinkDAQSourceTestFixture() {}

  void setup()
  {
    iomanager::ConnectionIds_t connections;
    connections.emplace_back(
      iomanager::ConnectionId{ "dummy", iomanager::ServiceType::kQueue, "TimeSync", "queue://kFollyMPMCQueue:100" });
    
    get_iomanager()->configure(connections);
  }

  void teardown() {
      get_iomanager()->reset();
  }
};

BOOST_TEST_GLOBAL_FIXTURE(DAQSinkDAQSourceTestFixture);

BOOST_AUTO_TEST_CASE(Basics)
{
  using namespace std::chrono_literals;
  auto queue_ref = iomanager::ConnectionRef{ "queue", "dummy" };
  auto sink =  get_iom_sender<dfmessages::TimeSync>(queue_ref);
  auto source = get_iom_receiver<dfmessages::TimeSync>(queue_ref);

  const uint64_t clock_frequency_hz = 62'500'000; // NOLINT(build/unsigned)

  timinglibs::TimestampEstimator te(source, clock_frequency_hz);

  // allow previous run timesync queue drain check to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  // There's no valid timestamp yet, because no TimeSync messages have
  // been received. We should immediately return with kInterrupted
  std::atomic<bool> do_not_continue_flag{ false };
  BOOST_CHECK_EQUAL(te.wait_for_valid_timestamp(do_not_continue_flag),
                    timinglibs::TimestampEstimatorBase::kInterrupted);

  std::atomic<bool> continue_flag{ true };
  dfmessages::timestamp_t initial_ts = 1;
  dfmessages::TimeSync initial_time_sync(initial_ts);
  sink->send(initial_time_sync, 10ms);
  BOOST_CHECK_EQUAL(te.wait_for_valid_timestamp(continue_flag), timinglibs::TimestampEstimatorBase::kFinished);

  dfmessages::timestamp_t ts_now = te.get_timestamp_estimate();

  // Check that the timestamp has advanced a bit from the initial TimeSync, but not by "too much".
  auto too_much = clock_frequency_hz / 10; // 100 ms
  BOOST_CHECK_GE(ts_now, initial_ts);
  BOOST_CHECK_LT(ts_now, initial_ts + too_much);

  BOOST_CHECK_EQUAL(te.wait_for_timestamp(ts_now + clock_frequency_hz, continue_flag),
                    timinglibs::TimestampEstimatorBase::kFinished);

  ts_now = te.get_timestamp_estimate();
  BOOST_CHECK_EQUAL(te.wait_for_timestamp(ts_now + clock_frequency_hz, do_not_continue_flag),
                    timinglibs::TimestampEstimatorBase::kInterrupted);

  // Check that the timestamp doesn't go backwards
  dfmessages::timestamp_t ts1 = te.get_timestamp_estimate();
  BOOST_CHECK_GE(ts1, initial_ts);
  dfmessages::timestamp_t ts2 = te.get_timestamp_estimate();
  BOOST_CHECK_GE(ts2, ts1);
  // Check that the timestamp advances
  std::this_thread::sleep_for(10ms);
  dfmessages::timestamp_t ts3 = te.get_timestamp_estimate();
  BOOST_CHECK_GT(ts3, ts2);
}

BOOST_AUTO_TEST_SUITE_END()
