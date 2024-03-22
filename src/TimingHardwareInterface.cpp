/**
 * @file TimingHardwareInterface.cpp TimingHardwareInterface class
 * implementation
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "timinglibs/TimingHardwareInterface.hpp"
#include "timinglibs/dal/TimingHardwareManagerPDIParameters.hpp"
#include "appfwk/DAQModule.hpp"

#include "timinglibs/TimingIssues.hpp"
#include "coredal/Connection.hpp"

#include "appfwk/app/Nljs.hpp"
#include "logging/Logging.hpp"
#include "rcif/cmd/Nljs.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <regex>

namespace dunedaq {
namespace timinglibs {

inline void
resolve_environment_variables(std::string& input_string)
{
  static std::regex env_var_pattern("\\$\\{([^}]+)\\}");
  std::smatch match;
  while (std::regex_search(input_string, match, env_var_pattern)) {
    const char* s = getenv(match[1].str().c_str());
    const std::string env_var(s == nullptr ? "" : s);
    input_string.replace(match[0].first, match[0].second, env_var);
  }
}

TimingHardwareInterface::TimingHardwareInterface()
  : m_connections_file("")
  , m_connection_manager(nullptr)
{
}

void
TimingHardwareInterface::configure_uhal(const dunedaq::timinglibs::dal::TimingHardwareManagerPDIParameters* mdal)
{
  m_uhal_log_level = mdal->get_uhal_log_level();

  if (!m_uhal_log_level.compare("debug")) {
    uhal::setLogLevelTo(uhal::Debug());
  } else if (!m_uhal_log_level.compare("info")) {
    uhal::setLogLevelTo(uhal::Info());
  } else if (!m_uhal_log_level.compare("notice")) {
    uhal::setLogLevelTo(uhal::Notice());
  } else if (!m_uhal_log_level.compare("warning")) {
    uhal::setLogLevelTo(uhal::Warning());
  } else if (!m_uhal_log_level.compare("error")) {
    uhal::setLogLevelTo(uhal::Error());
  } else if (!m_uhal_log_level.compare("fatal")) {
    uhal::setLogLevelTo(uhal::Fatal());
  } else {
    throw InvalidUHALLogLevel(ERS_HERE, m_uhal_log_level);
  }

  m_connections_file = "";
  try {
    m_connection_manager = std::make_unique<uhal::ConnectionManager>("file://" + m_connections_file);
  } catch (const uhal::exception::FileNotFound& excpt) {
    std::stringstream message;
    message << m_connections_file << " not found. Has TIMING_SHARE been set?";
    throw UHALConnectionsFileIssue(ERS_HERE, message.str(), excpt);
  }
}

void
TimingHardwareInterface::scrap_uhal()
{
  m_connection_manager.reset(nullptr);
  m_connections_file="";
}

} // namespace timinglibs
} // namespace dunedaq

// Local Variables:
// c-basic-offset: 2
// End:
