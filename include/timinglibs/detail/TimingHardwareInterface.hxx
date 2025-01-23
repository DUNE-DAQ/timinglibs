namespace dunedaq::timinglibs {


template<class TIMING_DEV>
TIMING_DEV
TimingHardwareInterface::cast_timing_device(const uhal::Node* device_node)
{
  auto timing_device = dynamic_cast<TIMING_DEV>(device_node);
  if (!timing_device)
  {
    throw UHALDeviceClassIssue(ERS_HERE, "", typeid(TIMING_DEV).name(), typeid(*device_node).name());
  }
  return timing_device;
}


}