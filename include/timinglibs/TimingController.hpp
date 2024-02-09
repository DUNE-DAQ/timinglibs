/**
 * @file TimingController.hpp
 *
 * TimingController is a DAQModule implementation that
 * provides a base class for timing controller modules.
 *
 * This is part of the DUNE DAQ Software Suite, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGCONTROLLER_HPP_
#define TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGCONTROLLER_HPP_

#include "timinglibs/timingcmd/Structs.hpp"
#include "timinglibs/TimingIssues.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/app/Nljs.hpp"
#include "appfwk/app/Structs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"
#include "iomanager/Sender.hpp"
#include "iomanager/Receiver.hpp"
#include "utilities/WorkerThread.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
ERS_DECLARE_ISSUE(timinglibs,                                                                                         ///< Namespace
                  TimingEndpointNotReady,                                                                             ///< Issue class name
                  endpoint << " timing endpoint did not become ready in time. State 0x" << std::hex << state, ///< Message
                  ((std::string)endpoint)((uint)state)                                                                ///< Message parameters
)
namespace timinglibs {

template<typename T>
struct MobileAtomic
{
  std::atomic<T> atomic;

  MobileAtomic()
    : atomic(T())
  {}

  explicit MobileAtomic(T const& v)
    : atomic(v)
  {}
  explicit MobileAtomic(std::atomic<T> const& a)
    : atomic(a.load())
  {}

  virtual ~MobileAtomic() = default;

  MobileAtomic(MobileAtomic const& other)
    : atomic(other.atomic.load())
  {}

  MobileAtomic& operator=(MobileAtomic const& other)
  {
    atomic.store(other.atomic.load());
    return *this;
  }

  MobileAtomic(MobileAtomic&&) = default;
  MobileAtomic& operator=(MobileAtomic&&) = default;
};

typedef MobileAtomic<uint64_t> AtomicUInt64; // NOLINT(build/unsigned)

/**
 * @brief TimingController is a DAQModule implementation that
 * provides a base class for timing controller modules.
 */
class TimingController : public dunedaq::appfwk::DAQModule
{
public:
  /**
   * @brief TimingController Constructor
   * @param name Instance name for this TimingController instance
   */
  explicit TimingController(const std::string& name, uint number_hw_commands);

  TimingController(const TimingController&) = delete;            ///< TimingController is not copy-constructible
  TimingController& operator=(const TimingController&) = delete; ///< TimingController is not copy-assignable
  TimingController(TimingController&&) = delete;                 ///< TimingController is not move-constructible
  TimingController& operator=(TimingController&&) = delete;      ///< TimingController is not move-assignable

  void init(std::shared_ptr<appfwk::ModuleConfiguration>) override;

protected:
  // DAQModule commands
  virtual void do_configure(const nlohmann::json&);
  virtual void do_start(const nlohmann::json&) {}
  virtual void do_stop(const nlohmann::json&) {}
  virtual void do_scrap(const nlohmann::json&);

  template<class T, class... Vs>
  void configure_hardware_or_recover_state(const nlohmann::json& data, std::string timing_entity_description, Vs... args);

  // Configuration
  std::string m_hw_command_out_connection;
  std::chrono::milliseconds m_hw_cmd_out_timeout;
  using sink_t = dunedaq::iomanager::SenderConcept<timingcmd::TimingHwCmd>;
  std::shared_ptr<sink_t> m_hw_command_sender;
  std::string m_timing_device;
  std::string m_timing_session_name;
  using source_t = dunedaq::iomanager::ReceiverConcept<nlohmann::json>;
  std::shared_ptr<source_t> m_device_info_receiver;

  virtual void send_hw_cmd(timingcmd::TimingHwCmd&& hw_cmd);
  virtual void send_configure_hardware_commands(const nlohmann::json& data) = 0;

  // opmon
  uint m_number_hw_commands;
  std::vector<AtomicUInt64> m_sent_hw_command_counters;

  // Interpert device opmon info
  virtual void process_device_info(nlohmann::json /*message*/) = 0;
  std::chrono::milliseconds m_device_ready_timeout;
  std::atomic<bool> m_device_ready;
  std::atomic<uint> m_device_infos_received_count;
  std::atomic<bool> m_hardware_state_recovery_enabled;
};
} // namespace timinglibs
} // namespace dunedaq

#include "detail/TimingController.hxx"

#endif // TIMINGLIBS_INCLUDE_TIMINGLIBS_TIMINGCONTROLLER_HPP_

// Local Variables:
// c-basic-offset: 2
// End:
