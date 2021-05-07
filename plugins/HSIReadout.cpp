/**
 * @file HSIReadout.cpp HSIReadout class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIReadout.hpp"

#include "timinglibs/hsireadout/Nljs.hpp"

#include "timinglibs/TimingIssues.hpp"

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

HSIReadout::HSIReadout(const std::string& name)
  : dunedaq::timinglibs::HSIInterface(name, std::bind(&HSIReadout::read_hsievents, this, std::placeholders::_1))
  , m_connections_file("")
  , m_connection_manager(nullptr)
  , m_hsi_device(nullptr)
{
  register_command("conf",  &HSIReadout::do_configure);
  register_command("start", &HSIReadout::do_start);
  register_command("stop",  &HSIReadout::do_stop);
  register_command("scrap", &HSIReadout::do_scrap);
}

void
HSIReadout::get_info(opmonlib::InfoCollector& /*ci*/, int /*level*/)
{
}

void
HSIReadout::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";

  m_cfg = obj.get<hsireadout::ConfParams>();
  
  m_connections_file = m_cfg.connections_file;

  TLOG_DEBUG(0) << get_name() << "conf: con. file before env var expansion: " << m_connections_file;
  resolve_environment_variables(m_connections_file);
  TLOG_DEBUG(0) << get_name() << "conf: con. file after env var expansion:  " << m_connections_file;

  if (!m_cfg.uhal_log_level.compare("debug"))
  {
    uhal::setLogLevelTo ( uhal::Debug() );
  }
  else if (!m_cfg.uhal_log_level.compare("info"))
  {
    uhal::setLogLevelTo ( uhal::Info() );
  }
  else if (!m_cfg.uhal_log_level.compare("notice"))
  {
    uhal::setLogLevelTo ( uhal::Notice() );
  }
  else if (!m_cfg.uhal_log_level.compare("warning"))
  {
    uhal::setLogLevelTo ( uhal::Warning() );
  }
  else if (!m_cfg.uhal_log_level.compare("error"))
  {
    uhal::setLogLevelTo ( uhal::Error() );
  }  
  else if (!m_cfg.uhal_log_level.compare("fatal"))
  {
    uhal::setLogLevelTo ( uhal::Fatal() );
  }
  else
  {
    throw InvalidUHALLogLevel(ERS_HERE, m_cfg.uhal_log_level);
  }

  try
  {
    m_connection_manager = std::make_unique< uhal::ConnectionManager >("file://"+m_connections_file);
  }
  catch (const uhal::exception::FileNotFound& excpt)
  {
    std::stringstream message;
    message << m_connections_file << " not found. Has TIMING_SHARE been set?";
    throw UHALConnectionsFileIssue(ERS_HERE, message.str(), excpt);
  }

  m_hsi_device_name = m_cfg.hsi_device_name;

  try
  {
    m_hsi_device = std::make_unique<uhal::HwInterface>( m_connection_manager->getDevice(m_hsi_device_name) );
  }
  catch (const uhal::exception::ConnectionUIDDoesNotExist& exception)
  { 
    std::stringstream message;
    message << "UHAL device name not " << m_hsi_device_name << " in connections file";
    throw UHALDeviceNameIssue(ERS_HERE, message.str(), exception);
  }
  
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
HSIReadout::read_hsievents(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(2) << get_name() << ": Entering read_hsievents() method";

  m_readout_counter = 0;
  m_sent_counter = 0;
  
  while (running_flag.load())
  {

    uint64_t local_read_counter=0;
    // we are assuming hsi already configured
    try
    {
      auto hsi_node = m_hsi_device->getNode<timing::HSINode>("endpoint0");

      if (hsi_node.read_buffer_error()) {
        ers::error(HSIBufferIssue(ERS_HERE, "ERROR"));
        continue;
      }

      if (hsi_node.read_buffer_warning()) {
        ers::warning(HSIBufferIssue(ERS_HERE, "WARNING"));
      }

      uint n_hsi_words = hsi_node.read_buffer_count();
    
      // this is bad
      if (n_hsi_words > 1024) {
        ers::error(HSIBufferIssue(ERS_HERE, "OVERFLOW"));
        continue;
      }

      TLOG_DEBUG(3) << get_name() << ": Number of words in HSI buffer: " << n_hsi_words;

      if (n_hsi_words >= 5) {
        auto hsi_words = hsi_node.read_data_buffer();

        uint n_hsi_events = hsi_words.size() / timing::g_hsi_event_size; 

        TLOG_DEBUG(2) << get_name() << ": Have readout " << n_hsi_events << " HSIEvent(s) ";

        m_readout_counter = m_readout_counter + n_hsi_events;
        local_read_counter = local_read_counter + n_hsi_events;
        for (uint i=0; i < n_hsi_events; ++i) {

          uint32_t header  = hsi_words.at(0+(i*timing::g_hsi_event_size));
          uint32_t ts_low  = hsi_words.at(1+(i*timing::g_hsi_event_size));
          uint32_t ts_high = hsi_words.at(2+(i*timing::g_hsi_event_size));
          uint32_t data    = hsi_words.at(3+(i*timing::g_hsi_event_size));
          uint32_t trigger = hsi_words.at(4+(i*timing::g_hsi_event_size));

          // put together the timestamp
          uint64_t ts = ts_low | (ts_high << 32);

          // bits 31-16 contain the HSI device ID
          uint32_t hsi_device_id = header >> 16;
          // bits 15-0 contain the sequence counter
          uint32_t counter = header & 0x0000ffff;

          if (counter > 0 && counter % 10000 == 0) TLOG_DEBUG(0) << "Sequence counter from firmware: " << counter;

          //TLOG_DEBUG(2) << get_name() << ": read out data: " << header << ", " << std::hex << ts << ", " << data << ", " << std::bitset<32>(trigger) << ", " << "\n";

          dfmessages::HSIEvent event = dfmessages::HSIEvent(hsi_device_id, trigger, ts, counter);

          send_hsi_event(event);
        }
      }
    }
    // TODO catch the correct exceptions here
    // network issue
    // node issue
    // queue sending issue
    catch (std::exception& excpt)
    {
      std::stringstream message;
      message << "Failed to get (HSI) node, endpoint0 from device name: " << m_hsi_device_name;
      ers::error(UHALDeviceNameIssue(ERS_HERE, message.str()));
      continue;
    }
  }
  std::ostringstream oss_summ;
  oss_summ << ": Exiting the read_hsievents() method, read out " << m_readout_counter
           << " HSIEvent messages and successfully sent " << m_sent_counter << " copies. ";
  ers::info(dunedaq::timinglibs::ProgressUpdate(ERS_HERE, get_name(), oss_summ.str()));
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

} // namespace timinglibs 
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::HSIReadout)

// Local Variables:
// c-basic-offset: 2
// End:
