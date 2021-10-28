/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_HSIREADOUT_HPP_
#define TIMINGLIBS_PLUGINS_HSIREADOUT_HPP_

#include "timinglibs/hsieventtofile/Nljs.hpp"
#include "timinglibs/hsieventtofile/Structs.hpp"

#include "timinglibs/hsieventtofileinfo/InfoNljs.hpp"
#include "timinglibs/hsieventtofileinfo/InfoStructs.hpp"

#include "TimingHardwareManager.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "timing/HSINode.hpp"

#include "dfmessages/HSIEvent.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

#include <ers/Issue.hpp>

#include <bitset>
#include <chrono>
#include <deque>
#include <memory>
#include <random>
#include <shared_mutex>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief HSIEventToFile generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class HSIEventToFile : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIEventToFile Constructor
   * @param name Instance name for this HSIEventToFile instance
   */
  explicit HSIEventToFile(const std::string& name);

  HSIEventToFile(const HSIEventToFile&) = delete;            ///< HSIEventToFile is not copy-constructible
  HSIEventToFile& operator=(const HSIEventToFile&) = delete; ///< HSIEventToFile is not copy-assignable
  HSIEventToFile(HSIEventToFile&&) = delete;                 ///< HSIEventToFile is not move-constructible
  HSIEventToFile& operator=(HSIEventToFile&&) = delete;      ///< HSIEventToFile is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  //hsireadout::ConfParams m_cfg;
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);

  dunedaq::appfwk::ThreadHelper m_thread;

  // Configuration
  using source_t = dunedaq::appfwk::DAQSource<dfmessages::HSIEvent>;
  std::unique_ptr<source_t> m_input_queue;

  std::chrono::milliseconds m_queue_timeout;
  
  hsieventtofile::ConfParams m_cfg;

  std::string m_output_file;
  std::shared_mutex m_ofs_mutex;
  std::ofstream m_ofs;
  
  void write_hsievents(std::atomic<bool>&);
  void write_hsievent(dfmessages::HSIEvent& event);

  std::atomic<uint64_t> m_received_counter;        // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_received_timestamp; // NOLINT(build/unsigned)

  std::atomic<uint64_t> m_written_counter;         // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_failed_to_write_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_last_written_timestamp;  // NOLINT(build/unsigned)

};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_HSIREADOUT_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
