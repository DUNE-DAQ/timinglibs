/**
 * @file TimingMasterControllerPDII.cpp TimingMasterControllerPDII class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingMasterControllerPDII.hpp"

#include "timinglibs/timingmastercontroller/Nljs.hpp"
#include "timinglibs/timingmastercontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

// void
// TimingMasterControllerPDII::process_device_info(nlohmann::json info)
// {
//   ++m_device_infos_received_count;

//   timing::timingfirmwareinfo::MasterMonitorData master_info;

//   auto master_data = info[opmonlib::JSONTags::children]["master"][opmonlib::JSONTags::properties][master_info.info_type][opmonlib::JSONTags::data];

//   from_json(master_data, master_info);

//   uint64_t master_timestamp = master_info.timestamp;
//   bool timestamp_broadcast_enabled = master_info.ts_en;
//   bool timestamp_error = master_info.ts_err;
//   bool transmit_error = master_info.tx_err;
//   bool counters_error = master_info.ctrs_rdy;

//   TLOG_DEBUG(3) << "Master timestamp: 0x" << std::hex << master_timestamp << ", ts_err: " << timestamp_error << ", tx_err: " << transmit_error << ", ctrs_rdy: " << counters_error << std::dec << ", infos received: " << m_device_infos_received_count;

//   if (master_timestamp && timestamp_broadcast_enabled && !timestamp_error && !transmit_error && counters_error)
//   {
//     if (!m_device_ready)
//     {
//       m_device_ready = true;
//       TLOG_DEBUG(2) << "Timing master became ready";
//     }
//   }
//   else
//   {
//     if (m_device_ready)
//     {
//       m_device_ready = false;
//       TLOG_DEBUG(2) << "Timing master no longer ready";
//     }
//   }
// }

} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingMasterControllerPDII)

// Local Variables:
// c-basic-offset: 2
// End:
