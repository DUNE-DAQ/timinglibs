/**
 * @file TimingMasterControllerPDI.cpp TimingMasterControllerPDI class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "TimingMasterControllerPDI.hpp"
#include "timinglibs/timingmastercontroller/Nljs.hpp"
#include "timinglibs/timingmastercontroller/Structs.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"

#include "timing/timingfirmwareinfo/InfoNljs.hpp"
#include "timing/timingfirmwareinfo/InfoStructs.hpp"

#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq {
namespace timinglibs {

void
TimingMasterControllerPDI::process_device_info(nlohmann::json info)
{
  ++m_device_infos_received_count;

  timing::timingfirmwareinfo::PDIMasterMonitorData master_info;

  auto master_data = info[opmonlib::JSONTags::children]["master"][opmonlib::JSONTags::properties][master_info.info_type][opmonlib::JSONTags::data];

  from_json(master_data, master_info);

  uint64_t master_timestamp = master_info.timestamp;
  
  TLOG_DEBUG(3) << "Master timestamp: 0x" << std::hex << master_timestamp << std::dec << ", infos received: " << m_device_infos_received_count;

  if (master_timestamp)
  {
    if (!m_device_ready)
    {
      m_device_ready = true;
      TLOG_DEBUG(2) << "Timing master became ready";
    }
  }
  else
  {
    if (m_device_ready)
    {
      m_device_ready = false;
      TLOG_DEBUG(2) << "Timing master no longer ready";
    }
  }
}
} // namespace timinglibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::timinglibs::TimingMasterControllerPDI)

// Local Variables:
// c-basic-offset: 2
// End:
