/**
 * @file MasterReadout.cpp MasterReadout class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "MasterReadout.hpp"

#include "timinglibs/hsireadout/Nljs.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
  ERS_DECLARE_ISSUE(timinglibs, InvalidEventHeader, " Invalid event header: " << std::showbase << std::hex << header, ((uint32_t)header))
  ERS_DECLARE_ISSUE(timinglibs, IncompleteEvent, " Invalid event, n words: " << n_words, ((uint32_t)n_words))
namespace timinglibs {

MasterReadout::MasterReadout(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&MasterReadout::read_hsievents, this, std::placeholders::_1))
  , m_hsievent_sink(nullptr)
  , m_queue_timeout(1)
  , m_readout_period(1000)
  , m_connections_file("")
  , m_connection_manager(nullptr)
  , m_hsi_device(nullptr)
  , m_readout_counter(0)
  , m_last_readout_timestamp(0)
  , m_sent_counter(0)
  , m_failed_to_send_counter(0)
  , m_last_sent_timestamp(0)

{
  register_command("conf", &MasterReadout::do_configure);
  register_command("start", &MasterReadout::do_start);
  register_command("stop", &MasterReadout::do_stop);
  register_command("scrap", &MasterReadout::do_scrap);
}

void
MasterReadout::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_hsievent_sink.reset(new appfwk::DAQSink<dfmessages::HSIEvent>(appfwk::queue_inst(init_data, "hsievent_sink")));

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
MasterReadout::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  m_cfg = obj.get<hsireadout::ConfParams>();

  m_connections_file = m_cfg.connections_file;
  m_readout_period = m_cfg.readout_period;

  TLOG_DEBUG(0) << get_name() << "conf: con. file before env var expansion: " << m_connections_file;
  resolve_environment_variables(m_connections_file);
  TLOG_DEBUG(0) << get_name() << "conf: con. file after env var expansion:  " << m_connections_file;

  if (!m_cfg.uhal_log_level.compare("debug")) {
    uhal::setLogLevelTo(uhal::Debug());
  } else if (!m_cfg.uhal_log_level.compare("info")) {
    uhal::setLogLevelTo(uhal::Info());
  } else if (!m_cfg.uhal_log_level.compare("notice")) {
    uhal::setLogLevelTo(uhal::Notice());
  } else if (!m_cfg.uhal_log_level.compare("warning")) {
    uhal::setLogLevelTo(uhal::Warning());
  } else if (!m_cfg.uhal_log_level.compare("error")) {
    uhal::setLogLevelTo(uhal::Error());
  } else if (!m_cfg.uhal_log_level.compare("fatal")) {
    uhal::setLogLevelTo(uhal::Fatal());
  } else {
    throw InvalidUHALLogLevel(ERS_HERE, m_cfg.uhal_log_level);
  }

  try {
    m_connection_manager = std::make_unique<uhal::ConnectionManager>("file://" + m_connections_file);
  } catch (const uhal::exception::FileNotFound& excpt) {
    std::stringstream message;
    message << m_connections_file << " not found. Has TIMING_SHARE been set?";
    throw UHALConnectionsFileIssue(ERS_HERE, message.str(), excpt);
  }

  m_hsi_device_name = m_cfg.hsi_device_name;

  try {
    m_hsi_device = std::make_unique<uhal::HwInterface>(m_connection_manager->getDevice(m_hsi_device_name));
  } catch (const uhal::exception::ConnectionUIDDoesNotExist& exception) {
    std::stringstream message;
    message << "UHAL device name not " << m_hsi_device_name << " in connections file";
    throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
  }

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
MasterReadout::do_start(const nlohmann::json& /*args*/)
{
  TLOG() << get_name() << ": Entering do_start() method";
  m_thread.start_working_thread("read-hsi-events");
  TLOG() << get_name() << " successfully started";
  TLOG() << get_name() << ": Exiting do_start() method";
}

void
MasterReadout::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
MasterReadout::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

void
MasterReadout::read_hsievents(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering read_hsievents() method";

  m_readout_counter = 0;
  m_sent_counter = 0;
  m_failed_to_send_counter = 0;

  m_last_readout_timestamp = 0;
  m_last_sent_timestamp = 0;

  while (running_flag.load()) {
    // we are assuming hsi already configured
    try {

      auto partition_node = m_hsi_device->getNode<timing::PartitionNode>("master.partition0");
      
      uint32_t n_words_in_buffer = partition_node.read_buffer_word_count(); // NOLINT(build/unsigned)
      update_buffer_counts(n_words_in_buffer);

      auto partition_words = partition_node.read_events();


      TLOG_DEBUG(5) << get_name() << ": Number of words in partition buffer: " << n_words_in_buffer;
      
      if ((partition_words.size() % timing::g_event_size) != 0) {
	ers::error(IncompleteEvent(ERS_HERE, partition_words.size()));
      } else {

        uint n_partition_events = partition_words.size() / timing::g_event_size;
	
        TLOG_DEBUG(4) << get_name() << ": Have readout " << n_partition_events << " Partition Event(s) ";

        m_readout_counter.store(m_readout_counter.load() + n_partition_events);
        for (uint i = 0; i < n_partition_events; ++i) {

          uint32_t header = partition_words.at(0 + (i * timing::g_event_size));  // NOLINT(build/unsigned)
          uint32_t ts_low = partition_words.at(2 + (i * timing::g_event_size));  // NOLINT(build/unsigned)
          uint32_t ts_high = partition_words.at(3 + (i * timing::g_event_size)); // NOLINT(build/unsigned)
          uint32_t data = 0x0;    // NOLINT(build/unsigned)
          uint32_t trigger = 0x0; // NOLINT(build/unsigned)

          // put together the timestamp
          uint64_t ts = ts_low | (static_cast<uint64_t>(ts_high) << 32); // NOLINT(build/unsigned)

          // bits 31-16 contain the HSI device ID
          uint32_t hsi_device_id = 0x0; // NOLINT(build/unsigned)
          // bits 15-0 contain the sequence counter
          uint32_t counter = partition_words.at(4 + (i * timing::g_event_size)); // NOLINT(build/unsigned)

          //if (counter > 0 && counter % 60000 == 0)
            TLOG_DEBUG(3) << "Sequence counter from firmware: " << counter;

          TLOG_DEBUG(3) << get_name() << ": read out data: " << std::showbase << std::hex << header << ", " << ts << ", " << data << ", "
                        << std::bitset<32>(trigger) << ", "
                        << "ts: " << ts
                        << "\n";
	  
	  if (header != 0xaa000600) {
	    ers::error(InvalidEventHeader(ERS_HERE,header));
	  }
          dfmessages::HSIEvent event = dfmessages::HSIEvent(header, trigger, ts, counter);
          m_last_readout_timestamp.store(ts);

          std::string thisQueueName = m_hsievent_sink->get_name();
          bool was_successfully_sent = false;
          while (!was_successfully_sent) {
            try {
              m_hsievent_sink->push(event, m_queue_timeout);
              ++m_sent_counter;
              m_last_sent_timestamp.store(event.timestamp);
              was_successfully_sent = true;
            } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
              std::ostringstream oss_warn;
              oss_warn << "push to output queue \"" << thisQueueName << "\"";
              ers::error(
                dunedaq::appfwk::QueueTimeoutExpired(ERS_HERE, get_name(), oss_warn.str(), m_queue_timeout.count()));
              ++m_failed_to_send_counter;
            }
          }
        }
      }
    } catch (const uhal::exception::ConnectionUIDDoesNotExist& excpt) {
      std::stringstream message;
      message << "Failed to get (HSI) node, endpoint0 from device name: " << m_hsi_device_name;
      ers::error(UHALDeviceNameIssue(ERS_HERE, message.str(), excpt));
      return;
    } catch (const uhal::exception::UdpTimeout& excpt) {
      ers::error(HSIReadoutNetworkIssue(ERS_HERE, excpt));
    } catch (const std::exception& excpt) {
      ers::error(HSIReadoutIssue(ERS_HERE, excpt));
    }
    std::this_thread::sleep_for(std::chrono::microseconds(m_readout_period));
  }
  std::ostringstream oss_summ;
  oss_summ << ": Exiting the read_hsievents() method, read out " << m_readout_counter.load()
           << " HSIEvent messages and successfully sent " << m_sent_counter.load() << " copies. ";
  ers::info(dunedaq::timinglibs::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

void
MasterReadout::update_buffer_counts(uint16_t new_count) // NOLINT(build/unsigned)
{
  std::unique_lock mon_data_lock(m_buffer_counts_mutex);
  if (m_buffer_counts.size() > 1000)
    m_buffer_counts.pop_front();
  m_buffer_counts.push_back(new_count);
}

double
MasterReadout::read_average_buffer_counts()
{
  std::unique_lock mon_data_lock(m_buffer_counts_mutex);

  double total_counts;
  uint32_t number_of_counts; // NOLINT(build/unsigned)

  total_counts = 0;
  number_of_counts = m_buffer_counts.size();

  if (number_of_counts) {
    for (uint i = 0; i < number_of_counts; ++i) { // NOLINT(build/unsigned)
      total_counts = total_counts + m_buffer_counts.at(i);
    }
    return total_counts / number_of_counts;
  } else {
    return 0;
  }
}

void
MasterReadout::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  hsireadoutinfo::Info module_info;

  module_info.readout_hsi_events_counter = m_readout_counter.load();
  module_info.sent_hsi_events_counter = m_sent_counter.load();
  module_info.failed_to_send_hsi_events_counter = m_failed_to_send_counter.load();

  module_info.last_readout_timestamp = m_last_readout_timestamp.load();
  module_info.last_sent_timestamp = m_last_sent_timestamp.load();

  module_info.average_buffer_occupancy = read_average_buffer_counts();

  ci.add(module_info);
}

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::MasterReadout)

// Local Variables:
// c-basic-offset: 2
// End:
