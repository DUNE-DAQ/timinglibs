# testapp_noreadout_two_process.py

# This python configuration produces *two* json configuration files
# that together form a MiniDAQApp with the same functionality as
# MiniDAQApp v1, but in two processes. One process contains the
# TriggerDecisionEmulator, while the other process contains everything
# else.
#
# As with testapp_noreadout_confgen.py
# in this directory, no modules from the readout package are used: the
# fragments are provided by the FakeDataProd module from dfmodules

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
def get_tmc_app(MASTER_DEVICE_NAME="",
                MASTER_SEND_DELAYS_PERIOD=0,
                MASTER_CLOCK_FILE="",
                MASTER_CLOCK_MODE=-1,
                MONITORED_ENDPOINTS=[],
                HOST="localhost",
                TIMING_HOST="np04-srv-012.cern.ch",
                TIMING_PORT=12345,
                DEBUG=False):
    
    modules = {}

    ## TODO all the connections...
    modules = [DAQModule(name = "tmc",
                        plugin = "TimingMasterController",
                        conf = tmc.ConfParams(
                                            device=MASTER_DEVICE_NAME,
                                            endpoint_scan_period=MASTER_SEND_DELAYS_PERIOD,
                                            clock_config=MASTER_CLOCK_FILE,
                                            fanout_mode=MASTER_CLOCK_MODE,
                                            monitored_endpoints=MONITORED_ENDPOINTS,
                                            ))]

    mgraph = ModuleGraph(modules)
    mgraph.add_external_connection("timing_cmds", "tmc.timing_cmds", Direction.OUT, TIMING_HOST, TIMING_PORT)
    mgraph.add_external_connection("timing_device_info", None, Direction.IN, TIMING_HOST, TIMING_PORT+1, [MASTER_DEVICE_NAME])
    
    tmc_app = App(modulegraph=mgraph, host=HOST, name="TMCApp")
    
    if DEBUG:
        tmc_app.export("tmc_app.dot")

    return tmc_app
