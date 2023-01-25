/**
 * @file TimingMasterControllerPDII.hpp
 *
 * TimingMasterControllerPDII is a DAQModule implementation that
 * provides a control interface for PDII timing master firmware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERPDII_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERPDII_HPP_

#include "TimingMasterController.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingmastercontroller/Nljs.hpp"
#include "timinglibs/timingmastercontroller/Structs.hpp"
#include "timinglibs/timingmastercontrollerinfo/InfoNljs.hpp"
#include "timinglibs/timingmastercontrollerinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief TimingMasterControllerPDII is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 */
class TimingMasterControllerPDII : public dunedaq::timinglibs::TimingMasterController
{
public:
  /**
   * @brief TimingMasterControllerPDII Constructor
   * @param name Instance name for this TimingMasterControllerPDII instance
   */
  explicit TimingMasterControllerPDII(const std::string& name) 
    : dunedaq::timinglibs::TimingMasterController(name) {}

  TimingMasterControllerPDII(const TimingMasterControllerPDII&) = delete; ///< TimingMasterControllerPDII is not copy-constructible
  TimingMasterControllerPDII& operator=(const TimingMasterControllerPDII&) =
    delete;                                                  ///< TimingMasterControllerPDII is not copy-assignable
  TimingMasterControllerPDII(TimingMasterControllerPDII&&) = delete; ///< TimingMasterControllerPDII is not move-constructible
  TimingMasterControllerPDII& operator=(TimingMasterControllerPDII&&) =
    delete; ///< TimingMasterControllerPDII is not move-assignable

protected:
  void process_device_info(nlohmann::json info) override;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGMASTERCONTROLLERPDII_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
