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

#include "InfoGatherer.hpp"
#include "timinglibs/TimingIssues.hpp"
#include "timinglibs/timingcmd/Nljs.hpp"
#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/timingcmd/msgp.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/app/Nljs.hpp"
#include "appfwk/app/Structs.hpp"
#include "ers/Issue.hpp"
#include "iomanager/Receiver.hpp"
#include "logging/Logging.hpp"

#include "timing/TimingNode.hpp"
#include "timing/EndpointDesignInterface.hpp"
#include "timing/HSIDesignInterface.hpp"
#include "timing/MasterDesignInterface.hpp"
#include "timing/TopDesignInterface.hpp"

#include "uhal/ConnectionManager.hpp"
#include "utilities/WorkerThread.hpp"

#include <map>
#include <memory>
#include <regex>
#include <string>
#include <vector>

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
  virtual ~TimingHardwareManager() {}
  
  void init(const nlohmann::json& init_data) override;
  virtual void conf(const nlohmann::json& conf_data);
  virtual void scrap(const nlohmann::json& data);

protected:
  // Commands
  //  virtual void do_configure(const nlohmann::json&);
  //  virtual void do_start(const nlohmann::json&);
  //  virtual void do_stop(const nlohmann::json&);
  //  virtual void do_scrap(const nlohmann::json&);

  virtual void process_hardware_command(timingcmd::TimingHwCmd& timing_hw_cmd);

  // Configuration
  std::string m_hw_cmd_connection;
  using source_t = dunedaq::iomanager::ReceiverConcept<timingcmd::TimingHwCmd>;
  std::shared_ptr<source_t> m_hw_command_receiver;
  std::string m_device_info_connection;
  iomanager::connection::ConnectionRef m_device_info_connection_ref;

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
  TIMING_DEV get_timing_device(const std::string& device_name);
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
  void send_fl_cmd(const timingcmd::TimingHwCmd& hw_cmd);
  void master_endpoint_scan(const timingcmd::TimingHwCmd& hw_cmd);

  // timing partition commands
  virtual void partition_configure(const timingcmd::TimingHwCmd& hw_cmd) = 0;

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

  void register_info_gatherer(uint gather_interval, const std::string& device_name, int op_mon_level, iomanager::connection::ConnectionRef info_connection);
  void gather_monitor_data(InfoGatherer& gatherer);

  virtual void start_hw_mon_gathering(const std::string& device_name = "");
  virtual void stop_hw_mon_gathering(const std::string& device_name = "");
  virtual std::vector<std::string> check_hw_mon_gatherer_is_running(const std::string& device_name);

  std::mutex m_command_threads_map_mutex;
  std::map<std::string, std::unique_ptr<dunedaq::utilities::ReusableThread>> m_command_threads;
  std::mutex master_sfp_mutex;
  virtual void perform_endpoint_scan(const timingcmd::TimingHwCmd& hw_cmd);
  virtual void clean_endpoint_scan_threads();
  std::unique_ptr<dunedaq::utilities::ReusableThread> m_endpoint_scan_threads_clean_up_thread;
  std::atomic<bool> m_run_clean_endpoint_scan_threads;
};

} // namespace timinglibs
} // namespace dunedaq

#include "detail/TimingHardwareManager.hxx"

#endif // TIMINGLIBS_SRC_TIMINGHARDWAREMANAGER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
