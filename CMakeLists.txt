cmake_minimum_required(VERSION 3.12)
project(timinglibs VERSION 4.0.2)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(coredal REQUIRED)
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
find_package(Boost COMPONENTS unit_test_framework REQUIRED)
find_package(okssystem REQUIRED)
find_package(oks REQUIRED)
find_package(genconfig REQUIRED)
find_package(oksdbinterfaces REQUIRED)

daq_oks_codegen(timing.schema.xml NAMESPACE dunedaq::timinglibs::dal DALDIR dal DEP_PKGS coredal)


daq_codegen( 
			 timingcmd.jsonnet
			 timingendpointcontroller.jsonnet
			 timinghardwaremanagerpdi.jsonnet
			 timingmastercontroller.jsonnet
			 timingpartitioncontroller.jsonnet
			 timingfanoutcontroller.jsonnet 
			 DEP_PKGS appfwk rcif cmdlib iomanager TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

daq_codegen( timingcmd.jsonnet TEMPLATES msgp.hpp.j2 )

daq_codegen(
			 timingendpointcontrollerinfo.jsonnet 
			 timinghardwaremanagerpdiinfo.jsonnet  
			 timingmastercontrollerinfo.jsonnet
			 timingpartitioncontrollerinfo.jsonnet
			 timingfanoutcontrollerinfo.jsonnet
			 DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

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
)

##############################################################################
daq_add_library(TimingController.cpp TimingHardwareInterface.cpp TimingHardwareManager.cpp TimingMasterController.cpp TimingHardwareInterface.cpp LINK_LIBRARIES ${TIMINGLIBS_DEPENDENCIES} oksdbinterfaces::oksdbinterfaces okssystem::okssystem
logging::logging coredal::coredal oks::oks ers::ers appfwk::appfwk)

##############################################################################
daq_add_plugin(TimingHardwareManagerPDII duneDAQModule LINK_LIBRARIES timing::timing timinglibs uhal::uhal pugixml::pugixml)
daq_add_plugin(TimingHardwareManagerPDI duneDAQModule LINK_LIBRARIES timing::timing timinglibs uhal::uhal pugixml::pugixml)
daq_add_plugin(TimingMasterControllerPDII duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingMasterControllerPDI duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingFanoutController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingPartitionController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingEndpointController duneDAQModule LINK_LIBRARIES timinglibs)

##############################################################################

##############################################################################
daq_install()
