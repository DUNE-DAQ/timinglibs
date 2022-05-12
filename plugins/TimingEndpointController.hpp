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

#include "TimingController.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingendpointcontroller/Nljs.hpp"
#include "timinglibs/timingendpointcontroller/Structs.hpp"
#include "timinglibs/timingendpointcontrollerinfo/InfoNljs.hpp"
#include "timinglibs/timingendpointcontrollerinfo/InfoStructs.hpp"

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
 * @brief TimingEndpointController is a DAQModule implementation that
 * provides that provides a control interface for a timing endpoint.
 */
class TimingEndpointController : public dunedaq::timinglibs::TimingController
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

private:
  uint m_managed_endpoint_id;

  // Commands
  void do_configure(const nlohmann::json& data) override;

  timingcmd::TimingHwCmd construct_endpoint_hw_cmd(const std::string& cmd_id);

  // timinglibs endpoint commands
  void do_endpoint_io_reset(const nlohmann::json& data);
  void do_endpoint_enable(const nlohmann::json& data);
  void do_endpoint_disable(const nlohmann::json&);
  void do_endpoint_reset(const nlohmann::json& data);
  void do_endpoint_print_status(const nlohmann::json&);
  void do_endpoint_print_timestamp(const nlohmann::json&);

  // pass op mon info
  void get_info(opmonlib::InfoCollector& ci, int level) override;
  void process_device_info(nlohmann::json info) override;
  uint32_t m_endpoint_state;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_TIMINGENDPOINTCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
