/**
 * @file TimingHardwareManager.hpp
 *
 * TimingHardwareManager is a DAQModule implementation that
 * provides the interface to the timing system hardware.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_SRC_TIMINGHARDWAREMANAGER_HPP_
#define TIMINGLIBS_SRC_TIMINGHARDWAREMANAGER_HPP_

#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingcmd/msgp.hpp"

#include "timinglibs/TimingIssues.hpp"

#include "InfoGatherer.hpp"

#include "timing/EndpointDesignInterface.hpp"
#include "timing/FanoutDesign.hpp"
#include "timing/HSIDesignInterface.hpp"
#include "timing/MasterDesignInterface.hpp"
#include "timing/TimingNode.hpp"
#include "timing/TopDesignInterface.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/DAQSource.hpp"
#include "utilities/WorkerThread.hpp"
#include "appfwk/app/Nljs.hpp"
#include "appfwk/app/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include "uhal/ConnectionManager.hpp"

#include "timing/EndpointNode.hpp"

#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace dunedaq {
namespace timinglibs {

void
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

/**
 * @brief TimingHardwareManager creates vectors of ints and writes
 * them to the configured output queues.
 */
class TimingHardwareManager : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief TimingHardwareManager Constructor
   * @param name Instance name for this TimingHardwareManager instance
   */
  explicit TimingHardwareManager(const std::string& name);

  TimingHardwareManager(const TimingHardwareManager&) = delete; ///< TimingHardwareManager is not copy-constructible
  TimingHardwareManager& operator=(const TimingHardwareManager&) =
    delete;                                                ///< TimingHardwareManager is not copy-assignable
  TimingHardwareManager(TimingHardwareManager&&) = delete; ///< TimingHardwareManager is not move-constructible
  TimingHardwareManager& operator=(TimingHardwareManager&&) = delete; ///< TimingHardwareManager is not move-assignable
  virtual ~TimingHardwareManager()
  {
    if (thread_.thread_running()) thread_.stop_working_thread();
  }
  void init(const nlohmann::json& init_data) override;
  virtual void conf(const nlohmann::json& conf_data);
  virtual void scrap(const nlohmann::json& data);

protected:
  // Commands
  //  virtual void do_configure(const nlohmann::json&);
  //  virtual void do_start(const nlohmann::json&);
  //  virtual void do_stop(const nlohmann::json&);
  //  virtual void do_scrap(const nlohmann::json&);

  // Threading
  dunedaq::utilities::WorkerThread thread_;
  virtual void process_hardware_commands(std::atomic<bool>&);

  // Configuration
  using source_t = dunedaq::appfwk::DAQSource<timingcmd::TimingHwCmd>;
  std::unique_ptr<source_t> m_hw_command_in_queue;
  std::chrono::milliseconds m_queue_timeout;

  // hardware polling intervals [us]
  uint m_gather_interval;
  uint m_gather_interval_debug;

  // uhal members
  std::string m_connections_file;
  std::string m_uhal_log_level;
  std::unique_ptr<uhal::ConnectionManager> m_connection_manager;
  std::map<std::string, std::unique_ptr<uhal::HwInterface>> m_hw_device_map;
  std::mutex m_hw_device_map_mutex;

  // retrieve top level/design object for a timing device
  template<class TIMING_DEV>
  const TIMING_DEV& get_timing_device(const std::string& device_name);
  const timing::TimingNode* get_timing_device_plain(const std::string& device_name);

  // timing hw cmds stuff
  std::map<timingcmd::TimingHwCmdId, std::function<void(const timingcmd::TimingHwCmd&)>> m_timing_hw_cmd_map_;

  template<typename Child>
  void register_timing_hw_command(const std::string& hw_cmd_id, void (Child::*f)(const timingcmd::TimingHwCmd&));

  // timing common commands
  void io_reset(const timingcmd::TimingHwCmd& hw_cmd);
  void print_status(const timingcmd::TimingHwCmd& hw_cmd);

  // timing master commands
  void set_timestamp(const timingcmd::TimingHwCmd& hw_cmd);
  void set_endpoint_delay(const timingcmd::TimingHwCmd& hw_cmd);

  // timing partition commands
  void partition_configure(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_enable(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_disable(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_start(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_stop(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_enable_triggers(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_disable_triggers(const timingcmd::TimingHwCmd& hw_cmd);
  void partition_print_status(const timingcmd::TimingHwCmd& hw_cmd);

  // timing endpoint commands
  void endpoint_enable(const timingcmd::TimingHwCmd& hw_cmd);
  void endpoint_disable(const timingcmd::TimingHwCmd& hw_cmd);
  void endpoint_reset(const timingcmd::TimingHwCmd& hw_cmd);

  // hsi
  void hsi_reset(const timingcmd::TimingHwCmd& hw_cmd);
  void hsi_configure(const timingcmd::TimingHwCmd& hw_cmd);
  void hsi_start(const timingcmd::TimingHwCmd& hw_cmd);
  void hsi_stop(const timingcmd::TimingHwCmd& hw_cmd);
  void hsi_print_status(const timingcmd::TimingHwCmd& hw_cmd);

  // opmon stuff
  std::atomic<uint64_t> m_received_hw_commands_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_accepted_hw_commands_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_rejected_hw_commands_counter; // NOLINT(build/unsigned)
  std::atomic<uint64_t> m_failed_hw_commands_counter;   // NOLINT(build/unsigned)

  // monitoring
  std::map<std::string, std::unique_ptr<InfoGatherer>> m_info_gatherers;

  void register_info_gatherer(uint gather_interval, const std::string& device_name, int op_mon_level);
  void gather_monitor_data(InfoGatherer& gatherer);

  virtual void start_hw_mon_gathering(const std::string& device_name = "");
  virtual void stop_hw_mon_gathering(const std::string& device_name = "");
  virtual std::vector<std::string> check_hw_mon_gatherer_is_running(const std::string& device_name);
};

} // namespace timinglibs
} // namespace dunedaq

#include "detail/TimingHardwareManager.hxx"

#endif // TIMINGLIBS_SRC_TIMINGHARDWAREMANAGER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
