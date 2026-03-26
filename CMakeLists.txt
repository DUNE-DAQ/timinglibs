cmake_minimum_required(VERSION 3.12)
project(timinglibs VERSION 4.7.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(confmodel REQUIRED)
find_package(ers REQUIRED)     
find_package(logging REQUIRED)
find_package(cmdlib REQUIRED)
find_package(rcif REQUIRED)
find_package(appfwk REQUIRED)
find_package(timing REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(opmonlib REQUIRED)
find_package(uhal REQUIRED)
find_package(pugixml REQUIRED)
find_package(iomanager REQUIRED)
find_package(conffwk REQUIRED)

daq_add_dal_library(timing.schema.xml NAMESPACE dunedaq::timinglibs::dal DALDIR dal DEP_PKGS confmodel)

#daq_codegen( timingcmd.jsonnet TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 msgp.hpp.j2 )


##############################################################################
# Dependency sets
set(TIMINGLIBS_DEPENDENCIES 
  appfwk::appfwk
  iomanager::iomanager
  ers::ers
  logging::logging
  iomanager::iomanager
  opmonlib::opmonlib
  nlohmann_json::nlohmann_json
  timing::timing
  uhal::uhal
  pugixml::pugixml
  confmodel::confmodel
)

##############################################################################
daq_add_library(TimingController.cpp TimingEndpointControllerBase.cpp TimingHardwareInterface.cpp TimingHardwareManagerBase.cpp TimingMasterControllerBase.cpp LINK_LIBRARIES ${TIMINGLIBS_DEPENDENCIES} timinglibs_dal)

##############################################################################
daq_add_plugin(TimingHardwareManagerPDII duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingMasterControllerPDII duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingEndpointController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingFanoutController duneDAQModule LINK_LIBRARIES timinglibs)

##############################################################################
daq_install()
