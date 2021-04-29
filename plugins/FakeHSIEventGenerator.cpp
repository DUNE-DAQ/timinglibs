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

//#include "timinglibs/Issues.hpp"

#include "appfwk/app/Nljs.hpp"
#include "appfwk/DAQModuleHelper.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <thread>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

FakeHSIEventGenerator::FakeHSIEventGenerator(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&FakeHSIEventGenerator::generate_hsievents, this, std::placeholders::_1))
  , m_hsievent_sink(nullptr)
  , m_time_sync_source(nullptr)
  , m_queue_timeout(100)
  , m_timestamp_estimator(nullptr)
  , m_random_generator()
  , m_uniform_distribution(0, UINT32_MAX)
  , m_clock_frequency(50e6)
  , m_event_period(20)
  , m_hsi_device_id(0)
  , m_signal_emulation_mode(0)
  , m_mean_signal_multiplicity(0)
  , m_enabled_signals(0)
  , m_generated_counter(0)
  , m_sent_counter(0)
{
  register_command("conf",  &FakeHSIEventGenerator::do_configure);
  register_command("start", &FakeHSIEventGenerator::do_start);
  register_command("stop",  &FakeHSIEventGenerator::do_stop);
  register_command("scrap", &FakeHSIEventGenerator::do_scrap);
}

void
FakeHSIEventGenerator::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_time_sync_source.reset(new appfwk::DAQSource<dfmessages::TimeSync>(appfwk::queue_inst(init_data,"time_sync_source")));
  m_hsievent_sink.reset(new appfwk::DAQSink<dfmessages::HSIEvent>(appfwk::queue_inst(init_data,"hsievent_sink")));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
FakeHSIEventGenerator::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{
}

void
FakeHSIEventGenerator::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  auto params = obj.get<fakehsieventgenerator::Conf>();

  m_clock_frequency = params.clock_frequency;
  m_event_period = params.event_period;
  m_hsi_device_id = params.hsi_device_id;  
  m_signal_emulation_mode = params.signal_emulation_mode;
  m_mean_signal_multiplicity = params.mean_signal_multiplicity;
  m_enabled_signals = params.enabled_signals;

  // configure the random distributions
  m_poisson_distribution = std::poisson_distribution<uint64_t>(m_mean_signal_multiplicity);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
FakeHSIEventGenerator::do_start(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";
  m_timestamp_estimator.reset(new TimestampEstimator(m_time_sync_source, m_clock_frequency));
  m_thread.start_working_thread("fake-tsd-gen");
  TLOG() << get_name() << " successfully started";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
}

void
FakeHSIEventGenerator::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
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

uint32_t FakeHSIEventGenerator::generate_signal_map() {

  uint32_t signal_map = 0;
  switch (m_signal_emulation_mode) {
    case 0:
      // 0b11111111 11111111 11111111 11111111
      signal_map = UINT32_MAX;
      break;
    case 1:
      for (uint i=0; i < 32; ++i) if (m_poisson_distribution(m_random_generator)) signal_map = signal_map | (1UL << i);
      break;
    case 2:
      signal_map = m_uniform_distribution(m_random_generator);
      break;
    default:
      signal_map = 0;
  }
  TLOG_DEBUG(3) << "raw gen. map: " << std::bitset<32>(signal_map) << ", masked gen. map:" << std::bitset<32>(signal_map & m_enabled_signals);
  return signal_map & m_enabled_signals;
}

void
FakeHSIEventGenerator::generate_hsievents(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering generate_hsievents() method";

  // Wait for there to be a valid timestsamp estimate before we start
  // TODO put in tome sort of timeout?
  if (m_timestamp_estimator->wait_for_valid_timestamp(running_flag) == TimestampEstimatorBase::kInterrupted) {
    ers::error(FailedToGetTimestampEstimate(ERS_HERE));
    return;
  }

  m_generated_counter = 0;
  m_sent_counter = 0;
  
  while (running_flag.load()) 
  {

    // emulate some signals
    uint32_t signal_map = generate_signal_map();
    
    // sleep for the configured event period
    std::this_thread::sleep_for(std::chrono::nanoseconds(m_event_period));

    // if at least one active signal, send a HSIEvent
    if (signal_map) {
      
      dfmessages::timestamp_t ts = m_timestamp_estimator->get_timestamp_estimate();
      
      ++m_generated_counter;

      dfmessages::HSIEvent event = dfmessages::HSIEvent(m_hsi_device_id, signal_map, ts, m_generated_counter);
      TLOG_DEBUG(1) << get_name() << ": Sending HSIEvent: " << event.header << ", " << std::bitset<32>(event.signal_map) << ", " << event.timestamp << ", " << event.sequence_counter <<"\n";

      std::string thisQueueName = m_hsievent_sink->get_name();
      bool successfullyWasSent = false;
      // do...while instead of while... so that we always try at least
      // once to send everything we generate, even if running_flag is
      // changed to false between the top of the main loop and here
      do
      {
        TLOG_DEBUG(2) << get_name() << ": Pushing the generated HSIEvent onto queue " << thisQueueName;
        try
        {
          m_hsievent_sink->push(event, m_queue_timeout);
          successfullyWasSent = true;
          ++m_sent_counter;
        }
        catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt)
        {
          std::ostringstream oss_warn;
          oss_warn << "push to output queue \"" << thisQueueName << "\"";
          ers::warning(dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
        }
      } while (!successfullyWasSent && running_flag.load());
    
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

} // namespace timinglibs 
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::FakeHSIEventGenerator)

// Local Variables:
// c-basic-offset: 2
// End: