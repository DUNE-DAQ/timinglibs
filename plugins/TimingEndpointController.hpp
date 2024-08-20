/**
 * @file TimingEndpointController.hpp
 *
 * TimingEndpointController is a DAQModule implementation that
 * provides that provides a control interface for a timing endpoint.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLER_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLER_HPP_

#include "TimingEndpointControllerBase.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingendpointcontroller/Nljs.hpp"
#include "timinglibs/timingendpointcontroller/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "utilities/WorkerThread.hpp"

#include "iomanager/Receiver.hpp"
#include "iomanager/Sender.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief TimingEndpointController is a DAQModule implementation that
 * provides that provides a control interface for a timing endpoint.
 */
class TimingEndpointController : public dunedaq::timinglibs::TimingEndpointControllerBase
{
public:
  /**
   * @brief TimingEndpointController Constructor
   * @param name Instance name for this TimingEndpointController instance
   */
  explicit TimingEndpointController(const std::string& name);

  TimingEndpointController(const TimingEndpointController&) =
    delete; ///< TimingEndpointController is not copy-constructible
  TimingEndpointController& operator=(const TimingEndpointController&) =
    delete;                                                      ///< TimingEndpointController is not copy-assignable
  TimingEndpointController(TimingEndpointController&&) = delete; ///< TimingEndpointController is not move-constructible
  TimingEndpointController& operator=(TimingEndpointController&&) =
    delete; ///< TimingEndpointController is not move-assignable
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
