/**
 * @file TimingEndpointControllerBase.hpp
 *
 * TimingEndpointControllerBase is a DAQModule implementation that
 * provides that provides a control interface for a timing endpoint.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLERBASE_HPP_
#define TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLERBASE_HPP_

#include "timinglibs/TimingController.hpp"

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
namespace timinglibs {

/**
 * @brief TimingEndpointControllerBase is a DAQModule implementation that
 * provides that provides a control interface for a timing endpoint.
 */
class TimingEndpointControllerBase : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief TimingEndpointControllerBase Constructor
   * @param name Instance name for this TimingEndpointControllerBase instance
   */
  explicit TimingEndpointControllerBase(const std::string& name, uint number_hw_commands);

  TimingEndpointControllerBase(const TimingEndpointControllerBase&) =
    delete; ///< TimingEndpointControllerBase is not copy-constructible
  TimingEndpointControllerBase& operator=(const TimingEndpointControllerBase&) =
    delete;                                                      ///< TimingEndpointControllerBase is not copy-assignable
  TimingEndpointControllerBase(TimingEndpointControllerBase&&) = delete; ///< TimingEndpointControllerBase is not move-constructible
  TimingEndpointControllerBase& operator=(TimingEndpointControllerBase&&) =
    delete; ///< TimingEndpointControllerBase is not move-assignable

protected:
  uint m_managed_endpoint_id;

  // Commands
  void do_configure(const nlohmann::json& data) override;
  void send_configure_hardware_commands(const nlohmann::json& data) override;

  timingcmd::TimingHwCmd construct_endpoint_hw_cmd(const std::string& cmd_id, uint endpoint_id);

  // timinglibs endpoint commands
  virtual void do_endpoint_enable(const nlohmann::json& data);
  virtual void do_endpoint_disable(const nlohmann::json& data);
  virtual void do_endpoint_reset(const nlohmann::json& data);

  // pass op mon info
  void process_device_info(nlohmann::json info) override;
  uint32_t m_endpoint_state;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLERBASE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
