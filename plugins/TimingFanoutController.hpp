/**
 * @file TimingFanoutController.hpp
 *
 * TimingFanoutController is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGFANOUTCONTROLLER_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGFANOUTCONTROLLER_HPP_

#include "timinglibs/TimingEndpointControllerBase.hpp"

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
                TimingFanoutNotReady,                                                                   ///< Issue class name
                fanout << " timing fanout did not become ready in time.", ///< Message
                ((std::string)fanout) )                                                                   ///< Message parameters
namespace timinglibs {

/**
 * @brief TimingFanoutController is a DAQModule implementation that
 * provides a control interface for timing master hardware.
 */
class TimingFanoutController : public dunedaq::timinglibs::TimingEndpointControllerBase
{
public:
  /**
   * @brief TimingFanoutController Constructor
   * @param name Instance name for this TimingFanoutController instance
   */
  explicit TimingFanoutController(const std::string& name);

  TimingFanoutController(const TimingFanoutController&) = delete; ///< TimingFanoutController is not copy-constructible
  TimingFanoutController& operator=(const TimingFanoutController&) =
    delete;                                                  ///< TimingFanoutController is not copy-assignable
  TimingFanoutController(TimingFanoutController&&) = delete; ///< TimingFanoutController is not move-constructible
  TimingFanoutController& operator=(TimingFanoutController&&) =
    delete; ///< TimingFanoutController is not move-assignable

protected:
  // Commands
  void do_configure(const nlohmann::json&) override;
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  timingcmd::TimingHwCmd construct_fanout_hw_cmd(const std::string& cmd_id);

  // pass op mon info
  void process_device_info(nlohmann::json info) override;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGFANOUTCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
