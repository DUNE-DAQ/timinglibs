namespace dunedaq::timinglibs {
template<class T, class... Vs>
void 
TimingController::configure_hardware_or_recover_state(const nlohmann::json& data, std::string timing_entity_description, Vs... args)
{
  bool conf_commands_sent=false;

 	if (!m_hardware_state_recovery_enabled)
  {
    TLOG_DEBUG(3) << "State recovery not enabled. Sending configure commands...";
    send_configure_hardware_commands(data);
    conf_commands_sent=true;
  }

  auto time_of_conf = std::chrono::high_resolution_clock::now();
  while (true)
  {
    auto now = std::chrono::high_resolution_clock::now();
    auto ms_since_conf = std::chrono::duration_cast<std::chrono::milliseconds>(now - time_of_conf);
    
    TLOG_DEBUG(3) << timing_entity_description << " (" << m_timing_device << ") ready: " << m_device_ready << ", infos received: " << m_device_infos_received_count;

    if (m_device_ready.load() && m_device_infos_received_count.load())
    {
      if (!conf_commands_sent)
      {
        TLOG_DEBUG(3) << "State recovered!";
      }
      break;
    }

    if (ms_since_conf > m_device_ready_timeout)
    {
      if (conf_commands_sent)
      {
        throw T(ERS_HERE,m_timing_device, args...);
      }
      else
      {
        TLOG_DEBUG(3) << "State not recovered! Sending configure commands...";
        send_configure_hardware_commands(data);
        time_of_conf = std::chrono::high_resolution_clock::now();
        conf_commands_sent=true;
      } 
    }
    TLOG_DEBUG(3) << "Waiting for " << timing_entity_description << " " << m_timing_device << " to become ready for (ms) " << ms_since_conf.count();
    std::this_thread::sleep_for(std::chrono::microseconds(250000));
  }
}

}