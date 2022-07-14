/**
 * @file FakeHSIEventGenerator.cpp FakeHSIEventGenerator class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "FakeHSIEventGenerator.hpp"

#include "timinglibs/fakehsieventgenerator/Nljs.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"
#include "dfmessages/HSIEvent.hpp"
#include "iomanager/IOManager.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

FakeHSIEventGenerator::FakeHSIEventGenerator(const std::string& name)
  : HSIEventSender(name)
  , m_timestamp_estimator(nullptr)
  , m_random_generator()
  , m_uniform_distribution(0, UINT32_MAX)
  , m_clock_frequency(50e6)
  , m_trigger_rate(1) // Hz
  , m_event_period(1e6) // us
  , m_timestamp_offset(0)
  , m_hsi_device_id(0)
  , m_signal_emulation_mode(0)
  , m_mean_signal_multiplicity(0)
  , m_enabled_signals(0)
  , m_generated_counter(0)
  , m_last_generated_timestamp(0)
{
  register_command("conf", &FakeHSIEventGenerator::do_configure);
  register_command("start", &FakeHSIEventGenerator::do_start);
  register_command("stop_trigger_sources", &FakeHSIEventGenerator::do_stop);
  register_command("scrap", &FakeHSIEventGenerator::do_scrap);
  register_command("change_rate", &FakeHSIEventGenerator::do_change_rate);
}

void
FakeHSIEventGenerator::init(const nlohmann::json& /*init_data*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
FakeHSIEventGenerator::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  fakehsieventgeneratorinfo::Info module_info;

  module_info.generated_hsi_events_counter = m_generated_counter.load();
  module_info.sent_hsi_events_counter = m_sent_counter.load();
  module_info.failed_to_send_hsi_events_counter = m_failed_to_send_counter.load();
  module_info.last_generated_timestamp = m_last_generated_timestamp.load();
  module_info.last_sent_timestamp = m_last_sent_timestamp.load();

  ci.add(module_info);
}

void
FakeHSIEventGenerator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  auto params = obj.get<fakehsieventgenerator::Conf>();

  m_hsievent_send_connection = params.hsievent_connection_name;

  m_clock_frequency = params.clock_frequency;
  if (params.trigger_rate>0)
    m_trigger_rate.store(params.trigger_rate);
  else
    ers::fatal(InvalidTriggerRateValue(ERS_HERE, params.trigger_rate));


  // time between HSI events [us]
  m_event_period.store(1.e6 / m_trigger_rate.load());
  TLOG() << get_name() << " Setting trigger rate, event period [us] to: " << m_trigger_rate.load() << ", "
         << m_event_period.load();

  // offset in units of clock ticks, positive offset increases timestamp
  m_timestamp_offset = params.timestamp_offset;
  m_hsi_device_id = params.hsi_device_id;
  m_signal_emulation_mode = params.signal_emulation_mode;
  m_mean_signal_multiplicity = params.mean_signal_multiplicity;
  m_enabled_signals = params.enabled_signals;
  m_timesync_topic = params.timesync_topic;

  // configure the random distributions
  m_poisson_distribution = std::poisson_distribution<uint64_t>(m_mean_signal_multiplicity); // NOLINT(build/unsigned)

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
FakeHSIEventGenerator::do_start(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_timestamp_estimator.reset(new TimestampEstimator(m_clock_frequency));

  m_received_timesync_count.store(0);

  m_timesync_receiver = get_iom_receiver<dfmessages::TimeSync>(m_timesync_topic);
  m_timesync_receiver->add_callback(std::bind(&FakeHSIEventGenerator::dispatch_timesync, this, std::placeholders::_1));

  auto start_params = obj.get<rcif::cmd::StartParams>();
  if (start_params.trigger_rate>0) {
    m_trigger_rate.store(start_params.trigger_rate);

    // time between HSI events [us]
    m_event_period.store(1.e6 / m_trigger_rate.load());
    TLOG() << get_name() << " Setting trigger rate, event period [us] to: " << m_trigger_rate.load() << ", "
           << m_event_period.load();
  } else {
    TLOG() << get_name() << " Using trigger rate, event period [us]: " << m_trigger_rate.load() << ", "
           << m_event_period.load();
  }
  m_run_number.store(start_params.run);

  m_thread.start_working_thread("fake-tsd-gen");
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
FakeHSIEventGenerator::do_change_rate(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_change_rate() method";

  auto change_rate_params = obj.get<rcif::cmd::ChangeRateParams>();
  TLOG() << get_name() << "trigger_RATE: " << change_rate_params.trigger_rate;
  m_trigger_rate.store(change_rate_params.trigger_rate);

  // time between HSI events [us]
  m_event_period.store(1.e6 / m_trigger_rate.load());
  TLOG() << get_name() << " Updating trigger rate, event period [us] to: " << m_trigger_rate.load() << ", "
         << m_event_period.load();

  TLOG() << get_name() << " successfully changed arate";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_change_rate() method";
}

void
FakeHSIEventGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();

  m_timesync_receiver->remove_callback();
  TLOG() << get_name() << ": received " << m_received_timesync_count.load() << " TimeSync messages.";

  m_timestamp_estimator.reset(nullptr); // Calls TimestampEstimator dtor
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
FakeHSIEventGenerator::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

uint32_t // NOLINT(build/unsigned)
FakeHSIEventGenerator::generate_signal_map()
{

  uint32_t signal_map = 0; // NOLINT(build/unsigned)
  switch (m_signal_emulation_mode) {
    case 0:
      // 0b11111111 11111111 11111111 11111111
      signal_map = UINT32_MAX;
      break;
    case 1:
      for (uint i = 0; i < 32; ++i)
        if (m_poisson_distribution(m_random_generator))
          signal_map = signal_map | (1UL << i);
      break;
    case 2:
      signal_map = m_uniform_distribution(m_random_generator);
      break;
    default:
      signal_map = 0;
  }
  TLOG_DEBUG(3) << "raw gen. map: " << std::bitset<32>(signal_map)
                << ", masked gen. map:" << std::bitset<32>(signal_map & m_enabled_signals);
  return signal_map & m_enabled_signals;
}

void
FakeHSIEventGenerator::do_hsievent_work(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering generate_hsievents() method";

  // Wait for there to be a valid timestsamp estimate before we start
  // TODO put in tome sort of timeout? Stoyan Trilov stoyan.trilov@cern.ch
  if (m_timestamp_estimator.get() != nullptr &&
      m_timestamp_estimator->wait_for_valid_timestamp(running_flag) == TimestampEstimatorBase::kInterrupted) {
    ers::error(FailedToGetTimestampEstimate(ERS_HERE));
    return;
  }

  m_generated_counter = 0;
  m_sent_counter = 0;
  m_last_generated_timestamp = 0;
  m_last_sent_timestamp = 0;
  m_failed_to_send_counter = 0;

  while (running_flag.load()) {

    // sleep for the configured event period, if trigger ticks are not 0, otherwise do not send anything
    if (m_trigger_rate.load() > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(m_event_period.load()));
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(250000));
      continue;
    }

    // emulate some signals
    uint32_t signal_map = generate_signal_map(); // NOLINT(build/unsigned)

    // if at least one active signal, send a HSIEvent
    if (signal_map && m_timestamp_estimator.get() != nullptr) {

      dfmessages::timestamp_t ts = m_timestamp_estimator->get_timestamp_estimate();

      ts += m_timestamp_offset;

      ++m_generated_counter;

      m_last_generated_timestamp.store(ts);

      dfmessages::HSIEvent event = dfmessages::HSIEvent(m_hsi_device_id, signal_map, ts, m_generated_counter, m_run_number);
      send_hsi_event(event);
    } else {
      continue;
    }
  }

  std::ostringstream oss_summ;
  oss_summ << ": Exiting the generate_hsievents() method, generated " << m_generated_counter
           << " HSIEvent messages and successfully sent " << m_sent_counter << " copies. ";
  ers::info(dunedaq::timinglibs::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

void
FakeHSIEventGenerator::dispatch_timesync(dfmessages::TimeSync& timesyncmsg)
{
  ++m_received_timesync_count;
  TLOG_DEBUG(13) << "Received TimeSync message with DAQ time= " << timesyncmsg.daq_time
                 << ", run=" << timesyncmsg.run_number << " (local run number is " << m_run_number << ")";
  if (m_timestamp_estimator.get() != nullptr) {
    if (timesyncmsg.run_number == m_run_number) {
      m_timestamp_estimator->add_timestamp_datapoint(timesyncmsg);
    } else {
      TLOG_DEBUG(0) << "Discarded TimeSync message from run " << timesyncmsg.run_number << " during run "
                    << m_run_number;
    }
  }
}

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::FakeHSIEventGenerator)

// Local Variables:
// c-basic-offset: 2
// End:
