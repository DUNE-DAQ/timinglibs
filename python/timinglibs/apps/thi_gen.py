# testapp_noreadout_two_process.py

# This python configuration produces *two* json configuration files
# that together form a MiniDAQApp with the same functionality as
# MiniDAQApp v1, but in two processes.  One process contains the
# TriggerDecisionEmulator, while the other process contains everything
# else.
#
# As with testapp_noreadout_confgen.py
# in this directory, no modules from the readout package are used: the
# fragments are provided by the FakeDataProd module from dfmodules


# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('timinglibs/timinghardwaremanagerpdi.jsonnet')
import dunedaq.timinglibs.timinghardwaremanagerpdi as thi

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction

#===============================================================================
def get_thi_app(FIRMWARE_TYPE='pdi',
                GATHER_INTERVAL=5e5,
                GATHER_INTERVAL_DEBUG=10e7,
                MASTER_DEVICE_NAME="",
                FANOUT_DEVICE_NAME="",
                HSI_DEVICE_NAME="",
                CONNECTIONS_FILE="${TIMING_SHARE}/config/etc/connections.xml",
                UHAL_LOG_LEVEL="notice",
                TIMING_PORT=12345,
                HOST="localhost",
                DEBUG=False):
    
    modules = {}
    fanout_devices=[]
    
    if FANOUT_DEVICE_NAME:
        fanout_devices.append(FANOUT_DEVICE_NAME)
    
    if FIRMWARE_TYPE == 'pdi':
        thi_class='TimingHardwareManagerPDI'
    elif FIRMWARE_TYPE == 'pdii':
        thi_class='TimingHardwareManagerPDII'
    else:
        raise Exception(f"'Unexpected firmware type: {FIRMWARE_TYPE}")

    modules = [ 
                DAQModule( name="thi",
                                plugin=thi_class,
                                conf= thi.ConfParams(connections_file=CONNECTIONS_FILE,
                                                       gather_interval=GATHER_INTERVAL,
                                                       gather_interval_debug=GATHER_INTERVAL_DEBUG,
                                                       monitored_device_name_master=MASTER_DEVICE_NAME,
                                                       monitored_device_names_fanout=fanout_devices,
                                                       monitored_device_name_endpoint="",
                                                       monitored_device_name_hsi=HSI_DEVICE_NAME,
                                                       uhal_log_level=UHAL_LOG_LEVEL)),
                ]                
        
    mgraph = ModuleGraph(modules)
    mgraph.add_external_connection("timing_cmds", "thi.timing_cmds", Direction.IN, HOST, TIMING_PORT)
    mgraph.add_external_connection("timing_device_info", "thi.timing_device_info", Direction.OUT, HOST, TIMING_PORT+1, set([MASTER_DEVICE_NAME,HSI_DEVICE_NAME,FANOUT_DEVICE_NAME]))

    thi_app = App(modulegraph=mgraph, host=HOST, name="THIApp")
    
    if DEBUG:
        thi_app.export("thi_app.dot")

    return thi_app
