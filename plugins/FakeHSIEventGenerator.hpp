/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_
#define TIMINGLIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

#include "timinglibs/fakehsieventgenerator/Nljs.hpp"
#include "timinglibs/fakehsieventgenerator/Structs.hpp"

#include "timinglibs/fakehsieventgeneratorinfo/InfoNljs.hpp"
#include "timinglibs/fakehsieventgeneratorinfo/InfoStructs.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "timinglibs/TimestampEstimator.hpp"

#include "dfmessages/HSIEvent.hpp"

#include "rcif/cmd/Nljs.hpp"
#include "rcif/cmd/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"
#include <ers/Issue.hpp>
#include <ipm/Receiver.hpp>

#include <bitset>
#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief FakeHSIEventGenerator generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class FakeHSIEventGenerator : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief FakeHSIEventGenerator Constructor
   * @param name Instance name for this FakeHSIEventGenerator instance
   */
  explicit FakeHSIEventGenerator(const std::string& name);

  FakeHSIEventGenerator(const FakeHSIEventGenerator&) = delete; ///< FakeHSIEventGenerator is not copy-constructible
  FakeHSIEventGenerator& operator=(const FakeHSIEventGenerator&) =
    delete;                                                ///< FakeHSIEventGenerator is not copy-assignable
  FakeHSIEventGenerator(FakeHSIEventGenerator&&) = delete; ///< FakeHSIEventGenerator is not move-constructible
  FakeHSIEventGenerator& operator=(FakeHSIEventGenerator&&) = delete; ///< FakeHSIEventGenerator is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);
  void do_resume(const nlohmann::json& obj);

  void dispatch_timesync(ipm::Receiver::Response message);

  // Threading
  dunedaq::appfwk::ThreadHelper m_thread;
  void generate_hsievents(std::atomic<bool>&);

  // Configuration
  using sink_t = dunedaq::appfwk::DAQSink<dfmessages::HSIEvent>;
  std::unique_ptr<sink_t> m_hsievent_sink;
  std::chrono::milliseconds m_queue_timeout;
  std::string m_timesync_topic;

  // Helper class for estimating DAQ time
  std::unique_ptr<TimestampEstimator> m_timestamp_estimator;

  // Random Generatior
  std::default_random_engine m_random_generator;
  std::uniform_int_distribution<uint32_t> m_uniform_distribution; // NOLINT(build/unsigned)
  std::poisson_distribution<uint64_t> m_poisson_distribution;     // NOLINT(build/unsigned)

  uint32_t generate_signal_map(); // NOLINT(build/unsigned)

  uint64_t m_clock_frequency;                     // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_trigger_interval_ticks; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_event_period;           // NOLINT(build/unsigned)
  int64_t m_timestamp_offset;

  uint32_t m_hsi_device_id;            // NOLINT(build/unsigned)
  uint m_signal_emulation_mode;        // NOLINT(build/unsigned)
  uint64_t m_mean_signal_multiplicity; // NOLINT(build/unsigned)

  uint32_t m_enabled_signals;                       // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_generated_counter;        // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_sent_counter;             // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_failed_to_send_counter;   // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_generated_timestamp; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_sent_timestamp;      // NOLINT(build/unsigned)

  std::atomic<uint64_t> m_received_timesync_count;  // NOLINT(build/unsigned)
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_FAKEHSIEVENTGENERATOR_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
