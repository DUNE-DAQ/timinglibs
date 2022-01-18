/**
 * @file HSIController.hpp
 *
 * HSIController is a DAQModule implementation that
 * provides that provides a control interface for a HSI endpoint.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_PLUGINS_HSICONTROLLER_HPP_
#define TIMINGLIBS_PLUGINS_HSICONTROLLER_HPP_

#include "TimingController.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timinglibs/hsicontroller/Nljs.hpp"
#include "timinglibs/hsicontroller/Structs.hpp"

#include "timinglibs/hsicontrollerinfo/InfoNljs.hpp"
#include "timinglibs/hsicontrollerinfo/InfoStructs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "utilities/WorkerThread.hpp"

#include "rcif/cmd/Nljs.hpp"
#include "rcif/cmd/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief HSIController is a DAQModule implementation that
 * provides that provides a control interface for a HSI endpoint.
 */
class HSIController : public dunedaq::timinglibs::TimingController
{
public:
  /**
   * @brief HSIController Constructor
   * @param name Instance name for this HSIController instance
   */
  explicit HSIController(const std::string& name);

  HSIController(const HSIController&) = delete;            ///< HSIController is not copy-constructible
  HSIController& operator=(const HSIController&) = delete; ///< HSIController is not copy-assignable
  HSIController(HSIController&&) = delete;                 ///< HSIController is not move-constructible
  HSIController& operator=(HSIController&&) = delete;      ///< HSIController is not move-assignable

  void init(const nlohmann::json& init_data) override;

private:
  hsicontroller::ConfParams m_hsi_configuration;

  // Commands
  void do_configure(const nlohmann::json& data) override;
  void do_start(const nlohmann::json& data) override;
  void do_stop(const nlohmann::json& data) override;
  void do_resume(const nlohmann::json& data);

  void construct_hsi_hw_cmd(timingcmd::TimingHwCmd& hw_cmd, const std::string& cmd_id);

  // timinglibs hsi commands
  void do_hsi_io_reset(const nlohmann::json& data);
  void do_hsi_endpoint_enable(const nlohmann::json& data);
  void do_hsi_endpoint_disable(const nlohmann::json&);
  void do_hsi_endpoint_reset(const nlohmann::json& data);

  void do_hsi_reset(const nlohmann::json&);
  void do_hsi_configure(const nlohmann::json& data);
  void do_hsi_start(const nlohmann::json&);
  void do_hsi_stop(const nlohmann::json&);

  void do_hsi_print_status(const nlohmann::json&);

  // pass op mon info
  void get_info(opmonlib::InfoCollector& ci, int level) override;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_PLUGINS_HSICONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
