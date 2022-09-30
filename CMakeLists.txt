cmake_minimum_required(VERSION 3.12)
project(timinglibs VERSION 2.1.1)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(ers REQUIRED)     
find_package(logging REQUIRED)
find_package(cmdlib REQUIRED)
find_package(rcif REQUIRED)
find_package(appfwk REQUIRED)
find_package(timing REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(dfmessages REQUIRED)
find_package(opmonlib REQUIRED)
find_package(uhal REQUIRED)
find_package(pugixml REQUIRED)
find_package(iomanager REQUIRED)
find_package(daqconf REQUIRED)
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

daq_codegen( 
			 timingcmd.jsonnet
			 timingendpointcontroller.jsonnet
			 timinghardwaremanagerpdi.jsonnet
			 timingmastercontroller.jsonnet
			 timingpartitioncontroller.jsonnet
			 timingfanoutcontroller.jsonnet 
			 DEP_PKGS appfwk rcif cmdlib iomanager TEMPLATES Structs.hpp.j2 Nljs.hpp.j2 )

daq_codegen( timingcmd.jsonnet TEMPLATES msgp.hpp.j2 )
daq_codegen( confgen.jsonnet DEP_PKGS daqconf TEMPLATES Structs.hpp.j2)

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
  dfmessages::dfmessages
  timing::timing
)

##############################################################################
daq_add_library(TimingController.cpp TimestampEstimatorBase.cpp TimestampEstimator.cpp TimestampEstimatorSystem.cpp TimingHardwareManager.cpp LINK_LIBRARIES ${TIMINGLIBS_DEPENDENCIES})

##############################################################################
daq_add_plugin(TimingHardwareManagerPDII duneDAQModule LINK_LIBRARIES timing::timing timinglibs uhal::uhal pugixml::pugixml)
daq_add_plugin(TimingHardwareManagerPDI duneDAQModule LINK_LIBRARIES timing::timing timinglibs uhal::uhal pugixml::pugixml)
daq_add_plugin(TimingMasterController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingFanoutController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingPartitionController duneDAQModule LINK_LIBRARIES timinglibs)
daq_add_plugin(TimingEndpointController duneDAQModule LINK_LIBRARIES timinglibs)

##############################################################################
daq_add_unit_test(TimestampEstimatorSystem_test  LINK_LIBRARIES timinglibs)
#daq_add_unit_test(TimestampEstimator_test        LINK_LIBRARIES timinglibs)

##############################################################################
daq_install()
