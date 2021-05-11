/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_SRC_HSIREADOUT_HPP_
#define TIMINGLIBS_SRC_HSIREADOUT_HPP_

#include "timinglibs/hsireadout/Structs.hpp"
#include "timinglibs/hsireadout/Nljs.hpp"

#include "timinglibs/hsireadoutinfo/Structs.hpp"
#include "timinglibs/hsireadoutinfo/Nljs.hpp"

#include "TimingHardwareManager.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "timing/HSINode.hpp"

#include "dfmessages/HSIEvent.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "uhal/ConnectionManager.hpp"
#include "uhal/utilities/files.hpp"
#include "uhal/log/exception.hpp"
#include "uhal/ProtocolUDP.hpp"

#include <ers/Issue.hpp>

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <bitset>
#include <shared_mutex>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief HSIReadout generates fake HSIEvent messages
 * and pushes them to the configured output queue.
 */
class HSIReadout : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief HSIReadout Constructor
   * @param name Instance name for this HSIReadout instance
   */
  explicit HSIReadout(const std::string& name);

  HSIReadout(const HSIReadout&) =
    delete; ///< HSIReadout is not copy-constructible
  HSIReadout& operator=(const HSIReadout&) =
    delete; ///< HSIReadout is not copy-assignable
  HSIReadout(HSIReadout&&) =
    delete; ///< HSIReadout is not move-constructible
  HSIReadout& operator=(HSIReadout&&) =
    delete; ///< HSIReadout is not move-assignable

  void init(const nlohmann::json& obj) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

private:
  // Commands
  hsireadout::ConfParams m_cfg;
  void do_configure(const nlohmann::json& obj);
  void do_start(const nlohmann::json& obj);
  void do_stop(const nlohmann::json& obj);
  void do_scrap(const nlohmann::json& obj);
  
  dunedaq::appfwk::ThreadHelper m_thread;

  // Configuration
  using sink_t = dunedaq::appfwk::DAQSink<dfmessages::HSIEvent>;
  std::unique_ptr<sink_t> m_hsievent_sink;

  std::chrono::milliseconds m_queue_timeout;
  std::string m_hsi_device_name;
  uint m_readout_period;

  std::string m_connections_file;
  std::unique_ptr<uhal::ConnectionManager> m_connection_manager;
  std::unique_ptr<uhal::HwInterface> m_hsi_device;

  void read_hsievents(std::atomic<bool>&);
  std::atomic<uint64_t> m_readout_counter;
  std::atomic<uint64_t> m_last_readout_timestamp;

  std::atomic<uint64_t> m_sent_counter;
  std::atomic<uint64_t> m_failed_to_send_counter;  
  std::atomic<uint64_t> m_last_sent_timestamp;
  
  std::deque<uint16_t> m_buffer_counts; 
  std::shared_mutex m_buffer_counts_mutex;
  void update_buffer_counts(uint16_t new_count);
  double read_average_buffer_counts();

};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_HSIREADOUT_HPP_

// Local Variables:
// c-basic-offset: 2
// End: