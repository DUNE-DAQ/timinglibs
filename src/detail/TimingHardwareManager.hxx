namespace dunedaq::timinglibs {

template<class Child>
void
TimingHardwareManager::register_timing_hw_command(const std::string& hw_cmd_id,
                                                  void (Child::*f)(const timingcmd::TimingHwCmd&))
{
  using namespace std::placeholders;

  std::string hw_cmd_name = hw_cmd_id;
  TLOG_DEBUG(0) << "Registering timing hw command id: " << hw_cmd_name << " called with " << typeid(f).name()
                << std::endl;

  bool done = m_timing_hw_cmd_map_.emplace(hw_cmd_name, std::bind(f, dynamic_cast<Child*>(this), _1)).second;
  if (!done) {
    throw TimingHardwareCommandRegistrationFailed(ERS_HERE, hw_cmd_name, get_name());
  }
}

template<class TIMING_DEV>
const TIMING_DEV&
TimingHardwareManager::get_timing_device(const std::string& device_name)
{
  return dynamic_cast<const TIMING_DEV&>(*get_timing_device_plain(device_name));
}

} // namespace dunedaq::timinglibs
