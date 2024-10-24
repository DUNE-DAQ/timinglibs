/**
 * @file TimingMasterControllerBase.hpp
 *
 * TimingMasterControllerBase is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERBASE_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERBASE_HPP_

#include "timinglibs/TimingController.hpp"
#include "timinglibs/dal/TimingMasterControllerConf.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
ERS_DECLARE_ISSUE(timinglibs,                                                                             ///< Namespace
                  TimingMasterNotReady,                                                                   ///< Issue class name
                  master << " timing master did not become ready in time.", ///< Message
                  ((std::string)master) )                                                                   ///< Message parameters
namespace timinglibs {

/**
 * @brief TimingMasterControllerBase is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 */
class TimingMasterControllerBase : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief TimingMasterControllerBase Constructor
   * @param name Instance name for this TimingMasterControllerBase instance
   */
  explicit TimingMasterControllerBase(const std::string& name);

  TimingMasterControllerBase(const TimingMasterControllerBase&) = delete; ///< TimingMasterControllerBase is not copy-constructible
  TimingMasterControllerBase& operator=(const TimingMasterControllerBase&) =
    delete;                                                  ///< TimingMasterControllerBase is not copy-assignable
  TimingMasterControllerBase(TimingMasterControllerBase&&) = delete; ///< TimingMasterControllerBase is not move-constructible
  TimingMasterControllerBase& operator=(TimingMasterControllerBase&&) =
    delete; ///< TimingMasterControllerBase is not move-assignable
  virtual ~TimingMasterControllerBase()
  {
    if (endpoint_scan_thread.thread_running())
      endpoint_scan_thread.stop_working_thread();
  }

protected:
  // Commands
  void do_configure(const nlohmann::json&) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  // timing master commands
  void do_master_set_timestamp(const nlohmann::json&);
  void do_master_set_endpoint_delay(const nlohmann::json& data);
  void do_master_send_fl_command(const nlohmann::json& data);
  void do_master_measure_endpoint_rtt(const nlohmann::json& data);
  void do_master_endpoint_scan(const nlohmann::json& data);

  // pass op mon info
  //  void get_info(opmonlib::InfoCollector& ci, int level) override;
  
  timingcmd::TimingEndpointLocations m_monitored_endpoint_locations;
  uint m_endpoint_scan_period; // NOLINT(build/unsigned)
  dunedaq::utilities::WorkerThread endpoint_scan_thread;
  virtual void endpoint_scan(std::atomic<bool>&);
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERBASE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
