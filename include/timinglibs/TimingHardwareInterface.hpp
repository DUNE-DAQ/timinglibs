/**
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGHARDWAREINTERFACE_HPP_
#define TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGHARDWAREINTERFACE_HPP_

#include "nlohmann/json.hpp"

#include "uhal/ConnectionManager.hpp"
#include "uhal/ProtocolUDP.hpp"
#include "uhal/log/exception.hpp"
#include "uhal/utilities/files.hpp"
#include <ers/Issue.hpp>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

/**
 * @brief TimingHardwareInterface sets up IPBus UHAL interface
 */
class TimingHardwareInterface
{
public:
  /**
   * @brief TimingHardwareInterface Constructor
   * @param name Instance name for this TimingHardwareInterface instance
   */
  explicit TimingHardwareInterface();

  TimingHardwareInterface(const TimingHardwareInterface&) = delete;            ///< TimingHardwareInterface is not copy-constructible
  TimingHardwareInterface& operator=(const TimingHardwareInterface&) = delete; ///< TimingHardwareInterface is not copy-assignable
  TimingHardwareInterface(TimingHardwareInterface&&) = delete;                 ///< TimingHardwareInterface is not move-constructible
  TimingHardwareInterface& operator=(TimingHardwareInterface&&) = delete;      ///< TimingHardwareInterface is not move-assignable

protected:
  void configure_uhal(const nlohmann::json& obj);

  void scrap_uhal (const nlohmann::json& obj);
  std::string m_connections_file;
  std::string m_uhal_log_level;
  std::unique_ptr<uhal::ConnectionManager> m_connection_manager;
};
} // namespace timinglibs
} // namespace dunedaq

#endif // TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGHARDWAREINTERFACE_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
