/**
 * @file TimingHardwareManagerPDI.hpp
 *
 * TimingHardwareManagerPDI is a DAQModule implementation that
 * provides the interface to the timing system hardware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGHARDWAREMANAGERPDI_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGHARDWAREMANAGERPDI_HPP_

#include "InfoGatherer.hpp"
#include "TimingHardwareManager.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Nljs.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Structs.hpp"
#include "timinglibs/timinghardwaremanagerpdiinfo/InfoNljs.hpp"
#include "timinglibs/timinghardwaremanagerpdiinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "timing/BoreasDesign.hpp"
#include "timing/ChronosDesign.hpp"
#include "timing/EndpointDesign.hpp"
#include "timing/FanoutDesign.hpp"
#include "timing/HSINode.hpp"
#include "timing/OuroborosDesign.hpp"
#include "timing/OuroborosMuxDesign.hpp"
#include "timing/OverlordDesign.hpp"
#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"
#include "uhal/ConnectionManager.hpp"
#include "uhal/utilities/files.hpp"
#include "utilities/WorkerThread.hpp"

#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief Hardware manager for PD-I hardware.
 */

class TimingHardwareManagerPDI : public TimingHardwareManager
{
public:
  /**
   * @brief TimingHardwareManagerPDI Constructor
   * @param name Instance name for this TimingHardwareManagerPDI instance
   */
  explicit TimingHardwareManagerPDI(const std::string& name);

  TimingHardwareManagerPDI(const TimingHardwareManagerPDI&) =
    delete; ///< TimingHardwareManagerPDI is not copy-constructible
  TimingHardwareManagerPDI& operator=(const TimingHardwareManagerPDI&) =
    delete;                                                      ///< TimingHardwareManagerPDI is not copy-assignable
  TimingHardwareManagerPDI(TimingHardwareManagerPDI&&) = delete; ///< TimingHardwareManagerPDI is not move-constructible
  TimingHardwareManagerPDI& operator=(TimingHardwareManagerPDI&&) =
    delete; ///< TimingHardwareManagerPDI is not move-assignable

  void conf(const nlohmann::json&) override;
  void start(const nlohmann::json& data);
  void stop(const nlohmann::json& data);

  void get_info(opmonlib::InfoCollector& ci, int level) override;

  // timing partition commands
  void partition_configure(const timingcmd::TimingHwCmd& hw_cmd) override;
  void partition_enable(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_disable(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_start(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_stop(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_enable_triggers(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_disable_triggers(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_print_status(const timingcmd::TimingHwCmd& hw_cmd);

protected:
  void register_common_hw_commands_for_design() override;
  void register_master_hw_commands_for_design() override;
  void register_endpoint_hw_commands_for_design() override;
  void register_hsi_hw_commands_for_design() override;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGHARDWAREMANAGERPDI_HPP_

// Local Variables:
// c-basic-offset: 2
// End: