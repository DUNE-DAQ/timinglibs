/**
 * @file InfoGatherer.hpp
 *
 * InfoGatherer is a DAQModule implementation that
 * provides the a mechanism of collecting and filling monitoring data in a dedicated thread.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_SRC_INFOGATHERER_HPP_
#define TIMINGLIBS_SRC_INFOGATHERER_HPP_

#include "timing/timingfirmwareinfo/Nljs.hpp"
#include "timing/timingfirmwareinfo/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include "iomanager/Sender.hpp"
#include "iomanager/IOManager.hpp"

#include "nlohmann/json.hpp"

#include <functional>
#include <future>
#include <list>
#include <memory>
#include <shared_mutex>
#include <string>

namespace dunedaq {

/**
 * @brief An ERS Issue raised when a threading state error occurs
 */
ERS_DECLARE_ISSUE(timinglibs,                                 // Namespace
                  GatherThreadingIssue,                       // Issue Class Name
                  "Gather Threading Issue detected: " << err, // Message
                  ((std::string)err))                         // Message parameters

ERS_DECLARE_ISSUE(timinglibs,
                  DeviceInfoSendFailed,
                  " Failed to send send " << device << " device info to " << destination << ".",
                  ((std::string)device)((std::string)destination))
namespace timinglibs {

/**
 * @brief InfoGatherer helper class for DAQ module monitor
 * data gathering.
 */
class InfoGatherer
{
public:
  /**
   * @brief InfoGatherer Constructor
   * @param gather_data function for data gathering
   * @param gather_interval interval for data gathering in us
   */
  explicit InfoGatherer(std::function<void(InfoGatherer&)> gather_data,
                        uint gather_interval,
                        const std::string& device_name,
                        int op_mon_level)
    : m_run_gathering(false)
    , m_gathering_thread(nullptr)
    , m_gather_interval(gather_interval)
    , m_device_name(device_name)
    , m_last_gathered_time(0)
    , m_op_mon_level(op_mon_level)
    , m_gather_data(gather_data)
    , m_device_info_connection_id(device_name+"_info")
    , m_hw_info_sender(nullptr)
    , m_sent_counter(0)
    , m_failed_to_send_counter(0)
    , m_queue_timeout(1)
  {
    //    m_info_collector = std::make_unique<opmonlib::InfoCollector>();
    m_hw_info_sender = iomanager::IOManager::get()->get_sender<nlohmann::json>(m_device_info_connection_id);
  }

  virtual ~InfoGatherer()
  {
    if (run_gathering()) stop_gathering_thread();
  }

  InfoGatherer(const InfoGatherer&) = delete;            ///< InfoGatherer is not copy-constructible
  InfoGatherer& operator=(const InfoGatherer&) = delete; ///< InfoGatherer is not copy-assignable
  InfoGatherer(InfoGatherer&&) = delete;                 ///< InfoGatherer is not move-constructible
  InfoGatherer& operator=(InfoGatherer&&) = delete;      ///< InfoGatherer is not move-assignable

  /**
   * @brief Start the monitoring thread (which executes the m_gather_data() function)
   * @throws MonitorThreadingIssue if the thread is already running
   */
  void start_gathering_thread(const std::string& name = "noname")
  {
    if (run_gathering()) {
      ers::warning(GatherThreadingIssue(ERS_HERE,
                                 "Attempted to start gathering thread "
                                 "when it is already supposed to be running!"));
      return;
    }
    m_run_gathering = true;
    m_gathering_thread.reset(new std::thread([&] { m_gather_data(*this); }));
    auto handle = m_gathering_thread->native_handle();
    auto rc = pthread_setname_np(handle, name.c_str());
    if (rc != 0) {
      std::ostringstream s;
      s << "The name " << name << " provided for the thread is too long.";
      ers::warning(GatherThreadingIssue(ERS_HERE, s.str()));
    }
  }

  /**
   * @brief Stop the gathering thread
   * @throws GatherThreadingIssue If the thread has not yet been started
   * @throws GatherThreadingIssue If the thread is not in the joinable state
   * @throws GatherThreadingIssue If an exception occurs during thread join
   */
  void stop_gathering_thread()
  {
    if (!run_gathering()) {
      ers::warning(GatherThreadingIssue(ERS_HERE,
                                 "Attempted to stop gathering thread "
                                 "when it is not supposed to be running!"));
      return;
    }
    m_run_gathering = false;
    if (m_gathering_thread->joinable()) {
      try {
        m_gathering_thread->join();
      } catch (std::system_error const& e) {
        throw GatherThreadingIssue(ERS_HERE, std::string("Error while joining gathering thread, ") + e.what());
      }
    } else {
      throw GatherThreadingIssue(ERS_HERE, "Thread not in joinable state during working thread stop!");
    }
  }

  /**
   * @brief Determine if the thread is currently running
   * @return Whether the thread is currently running
   */
  bool run_gathering() const { return m_run_gathering.load(); }

  void update_gather_interval(uint new_gather_interval) { m_gather_interval.store(new_gather_interval); }
  uint get_gather_interval() const { return m_gather_interval.load(); }

  void update_last_gathered_time(int64_t last_time) { m_last_gathered_time.store(last_time); }
  time_t get_last_gathered_time() const { return m_last_gathered_time.load(); }

  std::string get_device_name() const { return m_device_name; }

  int get_op_mon_level() const { return m_op_mon_level; }

  template<class DSGN>
  void collect_info_from_device(const DSGN& device)
  {
    std::unique_lock info_collector_lock(m_info_collector_mutex);
    m_device_info.reset( new timing::timingfirmwareinfo::TimingDeviceInfo() );
    device.get_info(*m_device_info);
    update_last_gathered_time(std::time(nullptr));
    send_device_info();
  }

  // void add_info_to_collector(std::string label, opmonlib::InfoCollector& ic)
  // {
  //   std::unique_lock info_collector_lock(m_info_collector_mutex);
  //   if (m_info_collector->is_empty()) {
  //     TLOG_DEBUG(3) << "skipping add info for gatherer: " << get_device_name()
  //            << " with gathered time: " << get_last_gathered_time() << " and level " << get_op_mon_level();
  //   } else {
  //     ic.add(label, *m_info_collector);
  //   }
  //   m_info_collector = std::make_unique<opmonlib::InfoCollector>();
  //   update_last_gathered_time(0);
  // }

private:
  void send_device_info()
  {
    //if (m_info_collector->is_empty())
    //{
    //  TLOG_DEBUG(3) << "skipping sending info for gatherer: " << get_device_name() << ", collector empty.";
    //  return;
    //}

    if (!m_hw_info_sender)
    {
      TLOG_DEBUG(3) << "skipping sending info for gatherer: " << get_device_name();
      return;
    }
    
    nlohmann::json info;
    to_json(info, *m_device_info);
    bool was_successfully_sent = false;
    while (!was_successfully_sent)
    {
      try
      {
        m_hw_info_sender->send(std::move(info), m_queue_timeout);
        TLOG_DEBUG(4) << "sent " << get_device_name() <<  " info";
        ++m_sent_counter;
        was_successfully_sent = true;
      }
      catch (const dunedaq::iomanager::TimeoutExpired& excpt)
      {
        ers::error(DeviceInfoSendFailed(ERS_HERE, m_device_name, m_device_info_connection_id));
        ++m_failed_to_send_counter;
      }
    }
  }

protected:
  std::atomic<bool> m_run_gathering;
  std::unique_ptr<std::thread> m_gathering_thread;
  std::atomic<uint> m_gather_interval;
  mutable std::shared_mutex m_mon_data_mutex;
  std::string m_device_name;
  std::atomic<time_t> m_last_gathered_time;
  int m_op_mon_level;
  //  std::unique_ptr<opmonlib::InfoCollector> m_info_collector;
  std::unique_ptr<timing::timingfirmwareinfo::TimingDeviceInfo> m_device_info;
  mutable std::mutex m_info_collector_mutex;
  std::function<void(InfoGatherer&)> m_gather_data;
  std::string m_device_info_connection_id;
  using sink_t = dunedaq::iomanager::SenderConcept<nlohmann::json>;
  std::shared_ptr<sink_t> m_hw_info_sender;
  std::atomic<uint> m_sent_counter;
  std::atomic<uint> m_failed_to_send_counter;
  std::chrono::milliseconds m_queue_timeout;
};

} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_INFOGATHERER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
