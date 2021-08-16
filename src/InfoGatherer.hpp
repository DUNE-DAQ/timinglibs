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

#include "InfoGathererInterface.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <functional>
#include <future>
#include <list>
#include <memory>
#include <shared_mutex>
#include <string>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief InfoGatherer helper class for DAQ module monitor
 * data gathering.
 */
template<class MON_DATA>
class InfoGatherer : public InfoGathererInterface
{
public:
  /**
   * @brief InfoGatherer Constructor
   * @param gather_data function for data gathering
   * @param gather_interval interval for data gathering in us
   */
  explicit InfoGatherer(std::function<void(InfoGatherer<MON_DATA>&)> gather_data,
                        uint gather_interval,
                        const std::string& device_name,
                        int op_mon_level)
    : InfoGathererInterface(gather_interval, device_name, op_mon_level)
    , m_gather_data(gather_data)
  {}

  InfoGatherer(const InfoGatherer&) = delete;            ///< InfoGatherer is not copy-constructible
  InfoGatherer& operator=(const InfoGatherer&) = delete; ///< InfoGatherer is not copy-assignable
  InfoGatherer(InfoGatherer&&) = delete;                 ///< InfoGatherer is not move-constructible
  InfoGatherer& operator=(InfoGatherer&&) = delete;      ///< InfoGatherer is not move-assignable

  /**
   * @brief Start the monitoring thread (which executes the m_gather_data() function)
   * @throws MonitorThreadingIssue if the thread is already running
   */
  void start_gathering_thread(const std::string& name = "noname") override
  {
    if (run_gathering()) {
      throw GatherThreadingIssue(ERS_HERE,
                                 "Attempted to start gathering thread "
                                 "when it is already supposed to be running!");
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

  void update_monitoring_data(MON_DATA& new_data)
  {
    std::unique_lock mon_data_lock(m_mon_data_mutex);
    m_mon_data = new_data;
  }

  MON_DATA get_monitoring_data() const
  {
    std::shared_lock mon_data_lock(m_mon_data_mutex);
    return m_mon_data;
  }

  void get_info(opmonlib::InfoCollector& ci, int level) const override
  {
    if (get_last_gathered_time() != 0 && get_op_mon_level() <= level) 
    {
      ci.add(get_monitoring_data());
    }
    else
    {
      TLOG_DEBUG(0) << "skipping gatherer for type: " << typeid(MON_DATA).name() << " for: " << get_device_name() << " with gathered time: " << get_last_gathered_time() 
      << ", gatherer level: " << get_op_mon_level() << ", opmon level: " << level;
    }
  }

  void get_info(nlohmann::json& data, int level) const override
  {
    if (get_last_gathered_time() != 0 && get_op_mon_level() <= level) 
    {
      data = get_monitoring_data();
    }
    else 
    {
      TLOG_DEBUG(0) << "skipping gatherer for type: " << typeid(MON_DATA).name() << " for: " << get_device_name() << " with gathered time: " << get_last_gathered_time() 
      << ", gatherer level: " << get_op_mon_level() << ", opmon level: " << level;
    }
  }

  nlohmann::json get_monitoring_data_json() const override
  {
    std::shared_lock mon_data_lock(m_mon_data_mutex);
    return m_mon_data;
  }
private:
  std::function<void(InfoGatherer<MON_DATA>&)> m_gather_data;
  MON_DATA m_mon_data;
};

} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_SRC_INFOGATHERER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
