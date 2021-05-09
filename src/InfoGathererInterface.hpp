/**
 * @file InfoGathererInterface.hpp
 *
 * InfoGathererInterface is a DAQModule implementation that
 * provides the a mechanism of collecting and filling monitoring data in a dedicated thread.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_SRC_INFOGATHERERINTERFACE_HPP_
#define TIMINGLIBS_SRC_INFOGATHERERINTERFACE_HPP_

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <functional>
#include <future>
#include <list>
#include <memory>
#include <string>
#include <shared_mutex>

namespace dunedaq {

/**
 * @brief An ERS Issue raised when a threading state error occurs
 */
ERS_DECLARE_ISSUE(timinglibs,                                      // Namespace
                  GatherThreadingIssue,                       // Issue Class Name
                  "Gather Threading Issue detected: " << err, // Message
                  ((std::string)err))                          // Message parameters

namespace timinglibs {

/**
 * @brief InfoGathererInterface helper class for DAQ module monitor
 * data gathering.
 */
class InfoGathererInterface
{
public:
  /**
   * @brief InfoGathererInterface Constructor
   * @param gather_data function for data gathering 
   * @param gather_interval interval for data gathering in us
   */
  explicit InfoGathererInterface(uint gather_interval, const std::string& device_name, uint op_mon_level)
    : m_run_gathering(false)
    , m_gathering_thread(nullptr)
    , m_gather_interval(gather_interval)
    , m_device_name(device_name)
    , m_last_gathered_time(0)
    , m_op_mon_level(op_mon_level)
  {}

  InfoGathererInterface(const InfoGathererInterface&) =
    delete; ///< InfoGathererInterface is not copy-constructible
  InfoGathererInterface& operator=(const InfoGathererInterface&) =
    delete; ///< InfoGathererInterface is not copy-assignable
  InfoGathererInterface(InfoGathererInterface&&) =
    delete; ///< InfoGathererInterface is not move-constructible
  InfoGathererInterface& operator=(InfoGathererInterface&&) =
    delete; ///< InfoGathererInterface is not move-assignable

  /**
   * @brief Start the monitoring thread (which executes the m_gather_data() function)
   * @throws MonitorThreadingIssue if the thread is already running
   */
  virtual void start_gathering_thread(const std::string& name="noname") = 0;
  /**
   * @brief Stop the gathering thread
   * @throws GatherThreadingIssue If the thread has not yet been started
   * @throws GatherThreadingIssue If the thread is not in the joinable state
   * @throws GatherThreadingIssue If an exception occurs during thread join
   */
  void stop_gathering_thread()
  {
    if (!run_gathering()) {
      throw GatherThreadingIssue(ERS_HERE,
                           "Attempted to stop gathering thread "
                           "when it is not supposed to be running!");
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
  uint get_last_gathered_time() const { return m_last_gathered_time.load(); }

  const std::string& get_device_name() {return m_device_name;}

  virtual const nlohmann::json get_monitoring_data() const = 0;

  uint get_op_mon_level() {return m_op_mon_level;}
protected:
  std::atomic<bool> m_run_gathering;
  std::unique_ptr<std::thread> m_gathering_thread;
  std::atomic<uint> m_gather_interval;
  mutable std::shared_mutex m_mon_data_mutex;
  std::string m_device_name;
  std::atomic<int64_t> m_last_gathered_time;
  uint m_op_mon_level;
};

} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_INFOGATHERERINTERFACE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
