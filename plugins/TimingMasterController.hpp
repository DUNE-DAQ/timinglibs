/**
 * @file TimingMasterController.hpp
 *
 * TimingMasterController is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLER_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLER_HPP_

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timinglibs/timingmastercontroller/Nljs.hpp"
#include "timinglibs/timingmastercontroller/Structs.hpp"

#include "timinglibs/timingmastercontrollerinfo/InfoNljs.hpp"
#include "timinglibs/timingmastercontrollerinfo/InfoStructs.hpp"

#include "TimingController.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "utilities/WorkerThread.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief TimingMasterController is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 */
class TimingMasterController : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief TimingMasterController Constructor
   * @param name Instance name for this TimingMasterController instance
   */
  explicit TimingMasterController(const std::string& name);

  TimingMasterController(const TimingMasterController&) = delete; ///< TimingMasterController is not copy-constructible
  TimingMasterController& operator=(const TimingMasterController&) =
    delete;                                                  ///< TimingMasterController is not copy-assignable
  TimingMasterController(TimingMasterController&&) = delete; ///< TimingMasterController is not move-constructible
  TimingMasterController& operator=(TimingMasterController&&) =
    delete; ///< TimingMasterController is not move-assignable
  virtual ~TimingMasterController()
  {
    if (set_endpoint_delay_thread.thread_running()) set_endpoint_delay_thread.stop_working_thread();
  }
private:
  // Commands
  void do_configure(const nlohmann::json& data) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  
  void construct_master_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id);

  // timing master commands
  void do_master_io_reset(const nlohmann::json& data);
  void do_master_set_timestamp(const nlohmann::json&);
  void do_master_print_status(const nlohmann::json&);

  // pass op mon info
  void get_info(opmonlib::InfoCollector& ci, int level) override;

  uint m_send_endpoint_delays_period; // NOLINT(build/unsigned)
  dunedaq::utilities::WorkerThread set_endpoint_delay_thread;
  virtual void set_endpoint_delay(std::atomic<bool>&);
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
