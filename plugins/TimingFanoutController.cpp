/**
 * @file TimingFanoutController.cpp TimingFanoutController class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingFanoutController.hpp"
#include "timinglibs/dal/TimingFanoutControllerConf.hpp"

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingfirmwareinfo/Nljs.hpp"
#include "timing/timingfirmwareinfo/Structs.hpp"

#include "timing/timingendpointinfo/Nljs.hpp"
#include "timing/timingendpointinfo/Structs.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

TimingFanoutController::TimingFanoutController(const std::string& name)
  : dunedaq::timinglibs::TimingEndpointControllerBase(name, 6) // 2nd arg: how many hw commands can this module send?
{
  register_command("conf", &TimingFanoutController::do_configure);
  register_command("start", &TimingFanoutController::do_start);
  register_command("stop", &TimingFanoutController::do_stop);
  register_command("scrap", &TimingFanoutController::do_scrap);
}

void
TimingFanoutController::do_configure(const nlohmann::json& data)
{
  auto mdal = m_params->cast<dal::TimingFanoutControllerConf>();
  
  m_device_ready_timeout = std::chrono::milliseconds(mdal->get_device_ready_timeout());

  TimingController::do_configure(data); // configure hw command connection

  configure_hardware_or_recover_state<TimingFanoutNotReady>(data, "Timing fanout");

  TLOG() << get_name() << "conf done for fanout device: " << m_timing_device;
}

// TODO: CHANGE
void
TimingFanoutController::send_configure_hardware_commands(const nlohmann::json& data)
{
  do_io_reset(data);
  std::this_thread::sleep_for(std::chrono::milliseconds(15000));
  do_endpoint_reset(data);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

//void
//TimingFanoutController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
//{
  // send counters internal to the module
//  timingfanoutcontrollerinfo::Info module_info;

//  module_info.sent_io_reset_cmds = m_sent_hw_command_counters.at(0).atomic.load();
//  module_info.sent_print_status_cmds = m_sent_hw_command_counters.at(1).atomic.load();
//  module_info.sent_fanout_endpoint_enable_cmds = m_sent_hw_command_counters.at(2).atomic.load();
//  module_info.sent_fanout_endpoint_reset_cmds = m_sent_hw_command_counters.at(4).atomic.load();

//   ci.add(module_info);
// }

void
TimingFanoutController::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;

  timing::timingfirmwareinfo::TimingDeviceInfo device_info;
  from_json(info, device_info);

  auto ept_info = device_info.endpoint_info;

  uint32_t endpoint_state = ept_info.state;
  bool ready = ept_info.ready;

  TLOG_DEBUG(3) << "state: 0x" << std::hex << endpoint_state << ", ready: " << ready << std::dec << ", infos received: " << m_device_infos_received_count;;

  if (endpoint_state > 0x5 && endpoint_state < 0x9)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "Timing fanout became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "Timing fanout no longer ready";
    }
  }
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingFanoutController)

// Local Variables:
// c-basic-offset: 2
// End:
