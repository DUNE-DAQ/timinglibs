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

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timinglibs/timinghardwaremanagerpdi/Nljs.hpp"
#include "timinglibs/timinghardwaremanagerpdi/Structs.hpp"

#include "timinglibs/timinghardwaremanagerpdiinfo/InfoNljs.hpp"
#include "timinglibs/timinghardwaremanagerpdiinfo/InfoStructs.hpp"

#include "TimingHardwareManager.hpp"

#include "InfoGatherer.hpp"
#include "InfoGathererInterface.hpp"

#include "timinglibs/TimingIssues.hpp"

// in timing-board-software at the moment
#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"

#include "timing/BoreasDesign.hpp"
#include "timing/EndpointDesign.hpp"
#include "timing/HSINode.hpp"
#include "timing/OuroborosDesign.hpp"
#include "timing/OverlordDesign.hpp"
#include "timing/OuroborosMuxDesign.hpp"
#include "timing/FanoutDesign.hpp"
#include "timing/ChronosDesign.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "appfwk/app/Nljs.hpp"
#include "appfwk/app/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include "uhal/ConnectionManager.hpp"
#include "uhal/utilities/files.hpp"

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

  void init(const nlohmann::json& init_data) override;
  void get_info(opmonlib::InfoCollector& ci, int level) override;

  // configuration
private:
  timinghardwaremanagerpdi::ConfParams m_cfg;
  void do_configure(const data_t& obj) override;

  ADD_VARIADIC_TEMPLATE_PROCESSOR_DECLARATIONS(register_common_hw_commands_for_design)
  ADD_VARIADIC_TEMPLATE_PROCESSOR_DECLARATIONS(register_master_hw_commands_for_design)
  ADD_VARIADIC_TEMPLATE_PROCESSOR_DECLARATIONS(register_endpoint_hw_commands_for_design)
  ADD_VARIADIC_TEMPLATE_PROCESSOR_DECLARATIONS(register_hsi_hw_commands_for_design)
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGHARDWAREMANAGERPDI_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
