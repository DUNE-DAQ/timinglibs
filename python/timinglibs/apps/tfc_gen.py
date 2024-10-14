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
moo.otypes.load_types('timinglibs/timingfanoutcontroller.jsonnet')
import dunedaq.timinglibs.timingfanoutcontroller as tfc

from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction

#===============================================================================
def get_tfc_app(FANOUT_DEVICE_NAME="",
                FANOUT_CLOCK_FILE="",
                FANOUT_CLOCK_SOURCE=1,
                TIMING_SYSTEM="",
                HARDWARE_STATE_RECOVERY_ENABLED=True,
                HOST="localhost",
                DEBUG=False):
    
    modules = {}

    ## TODO all the connections...
    modules = [DAQModule(name = "tfc",
                        plugin = "TimingFanoutController",
                        conf = tfc.ConfParams(
                                            device=FANOUT_DEVICE_NAME,
                                            hardware_state_recovery_enabled=HARDWARE_STATE_RECOVERY_ENABLED,
                                            timing_system_name=TIMING_SYSTEM,
                                            clock_config=FANOUT_CLOCK_FILE,
                                            clock_source=FANOUT_CLOCK_SOURCE,
                                            ))]

    mgraph = ModuleGraph(modules)
    
    mgraph.add_endpoint("timing_cmds", "tfc.timing_cmds", "TimingHwCmd", Direction.OUT)
    mgraph.add_endpoint(FANOUT_DEVICE_NAME+"_info", "tfc."+FANOUT_DEVICE_NAME+"_info", "JSON", Direction.IN, is_pubsub=True)
    
    tfc_app = App(modulegraph=mgraph, host=HOST, name="TFCApp")
    
    if DEBUG:
        tfc_app.export("tfc_app.dot")

    return tfc_app
