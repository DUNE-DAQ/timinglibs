/**
 * @file HSIEventToFile.cpp HSIEventToFile class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "HSIEventToFile.hpp"

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

ERS_DECLARE_ISSUE(timinglibs, BadFile, "Can not open file to store HSIEvent data: " << filename, ((std::string)filename))
ERS_DECLARE_ISSUE(timinglibs, BadFileOperation, "Can not write to file to store HSIEvent data: " << filename, ((std::string)filename))

namespace timinglibs {

HSIEventToFile::HSIEventToFile(const std::string& name)
  : dunedaq::appfwk::DAQModule(name)
  , m_thread(std::bind(&HSIEventToFile::write_hsievents, this, std::placeholders::_1))
  , m_input_queue(nullptr)
  , m_queue_timeout(1)
  , m_output_file("")
  , m_received_counter(0)
  , m_last_received_timestamp(0)
  , m_written_counter(0)
  , m_failed_to_write_counter(0)
  , m_last_written_timestamp(0)

{
  register_command("conf", &HSIEventToFile::do_configure);
  register_command("start", &HSIEventToFile::do_start);
  register_command("stop", &HSIEventToFile::do_stop);
  register_command("scrap", &HSIEventToFile::do_scrap);
}

void
HSIEventToFile::init(const nlohmann::json& init_data)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";

  m_input_queue.reset(new source_t(appfwk::queue_inst(init_data, "hsievent_source")));
  m_ofs.exceptions( std::ios_base::badbit | std::ios_base::failbit);

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
}

void
HSIEventToFile::do_configure(const nlohmann::json& obj)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
  
  m_cfg = obj.get<hsieventtofile::ConfParams>();

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";
}

void
HSIEventToFile::do_start(const nlohmann::json& /*args*/)
{
  TLOG() << get_name() << ": Entering do_start() method";
  
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);

  std::ostringstream file_name_stream;
  file_name_stream << m_cfg.file_path << "/" << m_cfg.file_prefix << std::put_time(&tm, "%d_%m_%Y_%H_%M_%S") << ".dat";
  m_output_file = file_name_stream.str();

  TLOG() << get_name() << ": Creating buffer for output file: " << file_name_stream.str();

  try
  {
    std::unique_lock mon_data_lock(m_ofs_mutex);
    m_ofs.open(file_name_stream.str(), std::ios::out | std::ios::app | std::ios::binary);
  }
  catch(const std::exception& e)
  {
    ers::error(BadFileOperation(ERS_HERE, m_output_file, e));
  }

  if (!m_ofs.is_open()) {
    ers::error(BadFile(ERS_HERE, m_output_file));
  }
  m_thread.start_working_thread("write-hsi-events");

  TLOG() << get_name() << " successfully started";
  TLOG() << get_name() << ": Exiting do_start() method";
}

void
HSIEventToFile::do_stop(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";
  m_thread.stop_working_thread();
  
  TLOG() << get_name() << ": Closing buffer for output file.";

  try
  {
    std::unique_lock mon_data_lock(m_ofs_mutex);
    m_ofs.close();
  }
  catch(const std::exception& e)
  {
    ers::error(BadFileOperation(ERS_HERE, m_output_file, e));
  }
  TLOG() << get_name() << " successfully stopped";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
}

void
HSIEventToFile::do_scrap(const nlohmann::json& /*args*/)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_scrap() method";
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_scrap() method";
}

void 
HSIEventToFile::write_hsievent(dfmessages::HSIEvent& event)
{
  std::unique_lock mon_data_lock(m_ofs_mutex);
  m_ofs.write((char*)&event.header, sizeof(event.header));
  m_ofs.write((char*)&event.signal_map, sizeof(event.signal_map));
  m_ofs.write((char*)&event.timestamp, sizeof(event.timestamp));
  m_ofs.write((char*)&event.sequence_counter, sizeof(event.sequence_counter));
  m_ofs.flush();
}

void
HSIEventToFile::write_hsievents(std::atomic<bool>& running_flag)
{
  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering write_hsievents() method";

  m_received_counter = 0;
  m_last_received_timestamp = 0;

  m_written_counter = 0;

  m_failed_to_write_counter = 0;
  m_last_written_timestamp = 0;

  while (true) {
    dfmessages::HSIEvent event;
    try {
      m_input_queue->pop(event, m_queue_timeout);
      ++m_received_counter;
    } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
      // The condition to exit the loop is that we've been stopped and
      // there's nothing left on the input queue
      if (!running_flag.load()) {
        break;
      } else {
        continue;
      }
    }
    
    m_last_received_timestamp = event.timestamp;
    TLOG_DEBUG(4) << "HSIEvent activity received.";
    
    try
    {
      write_hsievent(event);
      ++m_written_counter;
      m_last_written_timestamp = event.timestamp;
    }
    catch(const std::exception& e)
    {
      ers::error(BadFileOperation(ERS_HERE, m_output_file, e));
      ++m_failed_to_write_counter;
    }
  }

  TLOG() << "Received " << m_received_counter << " HSIEvent messages. Successfully written: " << m_written_counter;
  TLOG() << "Exiting do_work() method";

  TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_work() method";
}

void
HSIEventToFile::get_info(opmonlib::InfoCollector& ci, int /*level*/)
{
  // send counters internal to the module
  hsieventtofileinfo::Info module_info;

  module_info.received_counter = m_received_counter.load();
  module_info.last_received_timestamp = m_last_received_timestamp.load();

  module_info.written_counter = m_written_counter.load();
  module_info.failed_to_write_counter = m_failed_to_write_counter.load();
  module_info.last_written_timestamp = m_last_written_timestamp.load();

  ci.add(module_info);
}

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::HSIEventToFile)

// Local Variables:
// c-basic-offset: 2
// End:
