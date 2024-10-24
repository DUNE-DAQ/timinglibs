# testapp_noreadout_two_process.py

# This python configuration produces *two* json configuration files
# that together form a MiniDAQApp with the same functionality as
# MiniDAQApp v1, but in two processes. One process contains the
# TriggerDecisionEmulator, while the other process contains everything
# else.
#
# As with testapp_noreadout_confgen.py
# in this directory, no modules from the readout package are used: the
# fragments are provided by the FakeDataProdModule module from dfmodules

import math
from rich.console import Console
console = Console()

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('timinglibs/timingmastercontroller.jsonnet')
import dunedaq.timinglibs.timingmastercontroller as tmc

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction

#===============================================================================
def get_tmc_app(FIRMWARE_TYPE='pdii',
                MASTER_DEVICE_NAME="",
                MASTER_ENDPOINT_SCAN_PERIOD=0,
                MASTER_CLOCK_FILE="",
                MASTER_CLOCK_SOURCE=255,
                MONITORED_ENDPOINTS=[],
                TIMING_SESSION="",
                HARDWARE_STATE_RECOVERY_ENABLED=True,
                HOST="localhost",
                DEBUG=False):
    
    modules = {}

    if FIRMWARE_TYPE == 'pdi':
        tmc_class='TimingMasterControllerPDI'
    elif FIRMWARE_TYPE == 'pdii':
        tmc_class='TimingMasterControllerPDII'
    else:
        raise Exception(f"'Unexpected firmware type: {FIRMWARE_TYPE}")

    ## TODO all the connections...
    modules = [DAQModule(name = "tmc",
                        plugin = tmc_class,
                        conf = tmc.ConfParams(
                                            device=MASTER_DEVICE_NAME,
                                            hardware_state_recovery_enabled=HARDWARE_STATE_RECOVERY_ENABLED,
                                            timing_session_name=TIMING_SESSION,
                                            endpoint_scan_period=MASTER_ENDPOINT_SCAN_PERIOD,
                                            clock_config=MASTER_CLOCK_FILE,
                                            clock_source=MASTER_CLOCK_SOURCE,
                                            monitored_endpoints=MONITORED_ENDPOINTS,
                                            ))]

    mgraph = ModuleGraph(modules)
    mgraph.add_endpoint("timing_cmds", "tmc.timing_cmds", "TimingHwCmd", Direction.OUT)
    mgraph.add_endpoint(MASTER_DEVICE_NAME+"_info", "tmc."+MASTER_DEVICE_NAME+"_info", "JSON", Direction.IN, is_pubsub=True)
    
    tmc_app = App(modulegraph=mgraph, host=HOST, name="TMCApp")
    
    if DEBUG:
        tmc_app.export("tmc_app.dot")

    return tmc_app
