/**
 * @file TimingPartitionController.hpp
 *
 * TimingPartitionController is a DAQModule implementation that
 * provides that provides a control interface for a timing partition.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGPARTITIONCONTROLLER_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGPARTITIONCONTROLLER_HPP_

#include "timinglibs/TimingController.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingpartitioncontroller/Nljs.hpp"
#include "timinglibs/timingpartitioncontroller/Structs.hpp"
#include "timinglibs/timingpartitioncontrollerinfo/InfoNljs.hpp"
#include "timinglibs/timingpartitioncontrollerinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
ERS_DECLARE_ISSUE(timinglibs,                                                                             ///< Namespace
                  TimingPartitionNotReady,                                                                   ///< Issue class name
                  "Timing partitiion " << partition_id << " of timing master " << master << " did not become ready in time.", ///< Message
                  ((std::string)master)((uint)partition_id) )   
namespace timinglibs {

/**
 * @brief TimingPartitionController is a DAQModule implementation that
 * provides that provides a control interface for a timing partition.
 */
class TimingPartitionController : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief TimingPartitionController Constructor
   * @param name Instance name for this TimingPartitionController instance
   */
  explicit TimingPartitionController(const std::string& name);

  TimingPartitionController(const TimingPartitionController&) =
    delete; ///< TimingPartitionController is not copy-constructible
  TimingPartitionController& operator=(const TimingPartitionController&) =
    delete; ///< TimingPartitionController is not copy-assignable
  TimingPartitionController(TimingPartitionController&&) =
    delete; ///< TimingPartitionController is not move-constructible
  TimingPartitionController& operator=(TimingPartitionController&&) =
    delete; ///< TimingPartitionController is not move-assignable

private:
  uint m_managed_partition_id;

  // Commands
  void do_configure(const nlohmann::json& data) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  //  void do_scrap(const nlohmann::json& data);
  void do_resume(const nlohmann::json& data);
  void do_pause(const nlohmann::json& data);
  void do_scrap(const nlohmann::json& data);
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  uint16_t m_partition_trigger_mask;
  bool m_partition_control_rate_enabled;
  bool m_partition_spill_gate_enabled;


  timingcmd::TimingHwCmd construct_partition_hw_cmd(const std::string& cmd_id);

  // timing partition commands
  void do_partition_configure(const nlohmann::json& data);
  void do_partition_enable(const nlohmann::json&);
  void do_partition_disable(const nlohmann::json&);
  void do_partition_start(const nlohmann::json&);
  void do_partition_stop(const nlohmann::json&);
  void do_partition_enable_triggers(const nlohmann::json&);
  void do_partition_disable_triggers(const nlohmann::json&);
  void do_partition_print_status(const nlohmann::json&);

  // pass op mon info
  void get_info(opmonlib::InfoCollector& ci, int level) override;
  void process_device_info(nlohmann::json info) override;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGPARTITIONCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
