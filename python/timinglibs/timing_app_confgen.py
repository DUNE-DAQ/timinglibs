from rich.console import Console
console = Console()

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('timinglibs/timingcmd.jsonnet')
moo.otypes.load_types('timinglibs/timinghardwaremanagerpdi.jsonnet')
moo.otypes.load_types('timinglibs/timingmastercontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingfanoutcontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingpartitioncontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingendpointcontroller.jsonnet')
moo.otypes.load_types('hsilibs/hsicontroller.jsonnet')

# Import new types
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.timinglibs.timingcmd as tcmd
import dunedaq.timinglibs.timinghardwaremanagerpdi as thi
import dunedaq.timinglibs.timingmastercontroller as tmc
import dunedaq.timinglibs.timingpartitioncontroller as tpc
import dunedaq.timinglibs.timingendpointcontroller as tec
import dunedaq.hsilibs.hsicontroller as hsi
import dunedaq.timinglibs.timingfanoutcontroller as tfc

from appfwk.utils import acmd
from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction
from daqconf.core.system import System
from daqconf.core.metadata import write_metadata_file
from daqconf.core.config_file import generate_cli_from_schema

from timing.cli import toolbox

import json
import math

from os.path import exists, join

CLOCK_SPEED_HZ=62500000

def generate(
        CONFIG,
        RUN_NUMBER = 333, 
        PARTITION_IDS=[],
        FANOUT_DEVICES_NAMES=[],
        FANOUT_CLOCK_FILE="",
        ENDPOINT_CLOCK_FILE="",
        HSI_CLOCK_FILE="",
        HSI_RANDOM_RATE=1.0,
        PART_TRIGGER_MASK=0xff,
        PART_SPILL_GATE_ENABLED=True,
        PART_RATE_CONTROL_ENABLED=True,
        UHAL_LOG_LEVEL="notice",
        OUTPUT_PATH="./timing_app",
        DEBUG=False,
    ):
    
    if exists(OUTPUT_PATH):
        raise RuntimeError(f"Directory {OUTPUT_PATH} already exists")

    config_data = CONFIG[0]
    config_file = CONFIG[1]

    # Get our config objects
    # Loading this one another time... (first time in config_file.generate_cli_from_schema)
    moo.otypes.load_types('timinglibs/timing_app_confgen.jsonnet')
    import dunedaq.timinglibs.timing_app_confgen as timing_app_confgen
    
    moo.otypes.load_types('daqconf/bootgen.jsonnet')
    import dunedaq.daqconf.bootgen as daqconf_bootgen

    moo.otypes.load_types('daqconf/hsigen.jsonnet')
    import dunedaq.daqconf.hsigen as daqconf_hsigen

    moo.otypes.load_types('timinglibs/confgen.jsonnet')
    import dunedaq.timinglibs.confgen as daqconf_timing_confgen

    ## Hack, we shouldn't need to do that, in the future it should be, boot = config_data.boot
    boot = daqconf_bootgen.boot(**config_data.boot)
    if DEBUG: console.log(f"boot configuration object: {boot.pod()}")

    ## etc...
    thi_conf_gen = daqconf_timing_confgen.timing_hardware_interface(**config_data.timing_hardware_interface)
    if DEBUG: console.log(f"timing_hardware_interface configuration object: {thi_conf_gen.pod()}")

    tmc_conf_gen = daqconf_timing_confgen.timing_master_controller(**config_data.timing_master_controller)
    if DEBUG: console.log(f"tmc configuration object: {tmc_conf_gen.pod()}")

    tfc_conf_gen = daqconf_timing_confgen.timing_fanout_controller(**config_data.timing_fanout_controller)
    if DEBUG: console.log(f"timing_fanout_controller configuration object: {tfc_conf_gen.pod()}")

    hsi_conf_gen = daqconf_hsigen.hsi(**config_data.hsi)
    if DEBUG: console.log(f"hsi configuration object: {hsi_conf_gen.pod()}")

    tec_conf_gen = timing_app_confgen.timing_endpoint_controller(**config_data.timing_endpoint_controller)
    if DEBUG: console.log(f"timing_endpoint_controller configuration object: {tec_conf_gen.pod()}")
    
    FIRMWARE_STYLE = thi_conf_gen.firmware_type
    HSI_DEVICE_NAME = hsi_conf_gen.hsi_device_name
    ENDPOINT_DEVICE_NAME = tec_conf_gen.endpoint_device_name

    if FIRMWARE_STYLE == "pdi":
        thi_class="TimingHardwareManagerPDI"
    elif FIRMWARE_STYLE == "pdii":
        thi_class="TimingHardwareManagerPDII"
    else:
        console.log('!!! ERROR !!! Unknown firmware style. Exiting...', style="bold red")
        exit()

    # Define modules and queues
    thi_modules = [ 
                DAQModule( name="thi",
                                plugin=thi_class,
                                conf= thi.ConfParams(connections_file=thi_conf_gen.timing_hw_connections_file,
                                                       gather_interval=thi_conf_gen.gather_interval,
                                                       gather_interval_debug=thi_conf_gen.gather_interval_debug,
                                                       monitored_device_name_master=tmc_conf_gen.master_device_name,
                                                       monitored_device_names_fanout=FANOUT_DEVICES_NAMES,
                                                       monitored_device_name_endpoint=ENDPOINT_DEVICE_NAME,
                                                       monitored_device_name_hsi=HSI_DEVICE_NAME,
                                                       uhal_log_level=UHAL_LOG_LEVEL)),
                ]
    
    controller_modules = []
    custom_cmds=[]

    ## master and partition controllers
    master_controller_mod_name="tmc"
    
    if FIRMWARE_STYLE == "pdi":
        tmc_class="TimingMasterControllerPDI"
    elif FIRMWARE_STYLE == "pdii":
        tmc_class="TimingMasterControllerPDII"
    else:
        console.log('!!! ERROR !!! Unknown firmware style. Exiting...', style="bold red")
        exit()

    if tmc_conf_gen.master_device_name != "":
        controller_modules.extend( [DAQModule(name = master_controller_mod_name,
                    plugin = tmc_class,
                    conf = tmc.ConfParams(
                                        device=tmc_conf_gen.master_device_name,
                                        endpoint_scan_period=tmc_conf_gen.master_endpoint_scan_period,
                                        clock_config=tmc_conf_gen.master_clock_file,
                                        clock_source=tmc_conf_gen.master_clock_source,
                                        monitored_endpoints=tmc_conf_gen.monitored_endpoints,
                                        ))])


        custom_cmds.extend( [
                                ("master_configure", acmd([("tmc", tmc.ConfParams(
                                            device=tmc_conf_gen.master_device_name,
                                            endpoint_scan_period=tmc_conf_gen.master_endpoint_scan_period,
                                            clock_config=tmc_conf_gen.master_clock_file,
                                            fanout_mode=tmc_conf_gen.master_clock_mode,
                                            monitored_endpoints=tmc_conf_gen.monitored_endpoints,
                                                                ))])),
                                ("master_io_reset", acmd([("tmc", tcmd.IOResetCmdPayload(
                                                  clock_config=tmc_conf_gen.master_clock_file,
                                                  fanout_mode=tmc_conf_gen.master_clock_mode,
                                                  soft=False
                                                  ))])),
                                ("master_set_timestamp", acmd([("tmc",None)])),
                                ("master_print_status",  acmd([("tmc",None)])),
                            ] )
        if FIRMWARE_STYLE == 'pdii':
            custom_cmds.extend( [
                                    ("master_endpoint_scan", acmd([("tmc", tcmd.TimingMasterEndpointScanPayload(
                                                                    endpoints=tmc_conf_gen.monitored_endpoints,
                                                                        ))])),
                                ])

        ###
        if FIRMWARE_STYLE == "pdi":
            tpc_mods=[]
            for partition_id in PARTITION_IDS:

                tpc_mods.append( DAQModule(name = f"tpc{partition_id}",
                             plugin = "TimingPartitionController",
                             conf = tpc.PartitionConfParams(
                                                 device=tmc_conf_gen.master_device_name,
                                                 partition_id=partition_id,
                                                 trigger_mask=PART_TRIGGER_MASK,
                                                 spill_gate_enabled=PART_SPILL_GATE_ENABLED,
                                                 rate_control_enabled=PART_RATE_CONTROL_ENABLED,
                                                 )))
            custom_cmds.extend( [
                                    ("partition_configure", acmd([("tpc*", tpc.PartitionConfParams(
                                                                    device=tmc_conf_gen.master_device_name,
                                                                    partition_id=partition_id,
                                                                    trigger_mask=PART_TRIGGER_MASK,
                                                                    spill_gate_enabled=PART_SPILL_GATE_ENABLED,
                                                                    rate_control_enabled=PART_RATE_CONTROL_ENABLED,
                                                                ))])),
                                    ("partition_enable", acmd([("tpc.*", None)])),
                                    ("partition_disable", acmd([("tpc.*", None)])),
                                    ("partition_start", acmd([("tpc.*", None)])),
                                    ("partition_stop", acmd([("tpc.*", None)])),
                                    ("partition_enable_triggers", acmd([("tpc.*", None)])),
                                    ("partition_disable_triggers", acmd([("tpc.*", None)])),
                                    ("partition_print_status", acmd([("tpc.*", None)])),
                                ] )
            controller_modules.extend( tpc_mods )
        
        
    ## fanout controller
    for i,fanout_device_name in enumerate(FANOUT_DEVICES_NAMES):
        controller_modules.extend( [DAQModule(name = "tfc{}".format(i),
                        plugin = "TimingFanoutController",
                        conf = tfc.ConfParams(
                                            device=fanout_device_name,
                                            clock_config=FANOUT_CLOCK_FILE,
                                            ))]
                        )
        custom_cmds.extend( [
                            ("fanout_configure", acmd([("tfc*", tfc.ConfParams(
                                    device=fanout_device_name,
                                    ))])),

                            ("fanout_io_reset", acmd([("tfc.*", tcmd.IOResetCmdPayload(
                                                                  clock_config=FANOUT_CLOCK_FILE,
                                                                    soft=False
                                                    ))])),
                             ("fanout_print_status", acmd([("tfc.*", None)])),
                         ] )

    ## endpoint controllers
    ENDPOINT_ADDRESS = tec_conf_gen.endpoint_address
    ENDPOINT_PARTITION = tec_conf_gen.endpoint_partition

    if ENDPOINT_DEVICE_NAME != "":
        controller_modules.extend( [DAQModule(name = "tec0",
                        plugin = "TimingEndpointController",
                        conf = tec.ConfParams(
                                            device=ENDPOINT_DEVICE_NAME,
                                            endpoint_id=0,
                                            address=ENDPOINT_ADDRESS,
                                            partition=ENDPOINT_PARTITION
                                            ))] )
        custom_cmds.extend( [
                            
                            ("endpoint_io_reset", acmd([("tec.*", tcmd.IOResetCmdPayload(
                                                           clock_config=tec_conf_gen.endpoint_clock_file,
                                                            soft=False
                                                        ))])),


                            ("endpoint_enable", acmd([("tec.*", tcmd.TimingEndpointConfigureCmdPayload(
                                                                          address=ENDPOINT_ADDRESS,
                                                                          partition=ENDPOINT_PARTITION
                                                                          ))])),
                            ("endpoint_disable", acmd([("tec.*", None)])),

                            ("endpoint_reset",   acmd([("tec.*", tcmd.TimingEndpointConfigureCmdPayload(
                                                                          address=ENDPOINT_ADDRESS,
                                                                          partition=ENDPOINT_PARTITION
                                                                          ))])),
                            ("endpoint_print_status", acmd([("tec.*", None)])),

                            ])
    
    trigger_interval_ticks=0
    ## hsi controllers
    if HSI_DEVICE_NAME != "":
        if HSI_RANDOM_RATE > 0:
            trigger_interval_ticks=math.floor((1/HSI_RANDOM_RATE) * CLOCK_SPEED_HZ)
        else:
            console.log('WARNING! Emulated trigger rate of 0 will not disable signal emulation in real HSI hardware! To disable emulated HSI triggers, use  option: "--hsi-source 0" or mask all signal bits', style="bold red")
        
        startpars = rccmd.StartParams(run=RUN_NUMBER, trigger_rate = HSI_RANDOM_RATE)
        #resumepars = rccmd.ResumeParams(trigger_interval_ticks = trigger_interval_ticks)

        HSI_ENDPOINT_ADDRESS = hsi_conf_gen.hsi_endpoint_address
        HSI_ENDPOINT_PARTITION = hsi_conf_gen.hsi_endpoint_address
        HSI_RE_MASK = hsi_conf_gen.hsi_re_mask
        HSI_FE_MASK = hsi_conf_gen.hsi_fe_mask
        HSI_INV_MASK = hsi_conf_gen.hsi_inv_mask
        HSI_SOURCE = hsi_conf_gen.hsi_source
        

        controller_modules.extend( [ DAQModule(name="hsic",
                                plugin = "HSIController",
                                conf = hsi.ConfParams( device=HSI_DEVICE_NAME,
                                                        clock_frequency=CLOCK_SPEED_HZ,
                                                        trigger_rate=HSI_RANDOM_RATE,
                                                        address=HSI_ENDPOINT_ADDRESS,
                                                        partition=HSI_ENDPOINT_PARTITION,
                                                        rising_edge_mask=HSI_RE_MASK,
                                                        falling_edge_mask=HSI_FE_MASK,
                                                        invert_edge_mask=HSI_INV_MASK,
                                                        data_source=HSI_SOURCE),
                                extra_commands = {"start": startpars,
                                                  #"resume": resumepars
                                                  }), ] )
        custom_cmds.extend( [

                            ("hsi_io_reset", acmd([("hsi.*", tcmd.IOResetCmdPayload(
                                                                      clock_config=HSI_CLOCK_FILE,
                                                                      soft=False
                                                                      ))])),
                            ("hsi_endpoint_enable", acmd([("hsi.*", tcmd.TimingEndpointConfigureCmdPayload(
                                                                      address=HSI_ENDPOINT_ADDRESS,
                                                                      partition=HSI_ENDPOINT_PARTITION
                                                                      ))])),

                            ("hsi_endpoint_disable", acmd([("hsi.*", None)])),

                            ("hsi_endpoint_reset",   acmd([("hsi.*", tcmd.TimingEndpointConfigureCmdPayload(
                                                                          address=hsi_conf_gen.hsi_endpoint_address,
                                                                          partition=HSI_ENDPOINT_PARTITION
                                                                          ))])),
                            ("hsi_reset", acmd([("hsi.*", None)])),

                            ("hsi_configure", acmd([("hsi.*", tcmd.HSIConfigureCmdPayload(
                                                                  rising_edge_mask=HSI_RE_MASK,                   
                                                                  falling_edge_mask=HSI_FE_MASK,
                                                                  invert_edge_mask=HSI_INV_MASK,
                                                                  data_source=HSI_SOURCE,
                                                                  random_rate=HSI_RANDOM_RATE
                                                                  ))])),
                            ("hsi_start", acmd([("hsi.*", None)])),
                            ("hsi_stop", acmd([("hsi.*", None)])),
                            ("hsi_print_status", acmd([("hsi.*", None)])),
                        ])

    thi_graph = ModuleGraph(thi_modules)

    controllers_graph = ModuleGraph(controller_modules)
    if ENDPOINT_DEVICE_NAME:
        controllers_graph.add_endpoint("timing_cmds", "tec0.timing_cmds", "TimingHwCmd", Direction.OUT)
        
        controllers_graph.add_endpoint(ENDPOINT_DEVICE_NAME+"_info", f"tec0.{ENDPOINT_DEVICE_NAME}_info", "JSON", Direction.IN, is_pubsub=True)
        thi_graph.add_endpoint( ENDPOINT_DEVICE_NAME+"_info", "thi."+ENDPOINT_DEVICE_NAME+"_info", "JSON", Direction.OUT, is_pubsub=True)
    if tmc_conf_gen.master_device_name:
        controllers_graph.add_endpoint("timing_cmds", f"{master_controller_mod_name}.timing_cmds", "TimingHwCmd", Direction.OUT)
        
        controllers_graph.add_endpoint(tmc_conf_gen.master_device_name+"_info", f"{master_controller_mod_name}.{tmc_conf_gen.master_device_name}_info", "JSON", Direction.IN, is_pubsub=True)
        thi_graph.add_endpoint( tmc_conf_gen.master_device_name+"_info", "thi."+tmc_conf_gen.master_device_name+"_info", "JSON", Direction.OUT, is_pubsub=True)

        if FIRMWARE_STYLE == 'pdi':
            for partition_id in PARTITION_IDS:
                controllers_graph.add_endpoint("timing_cmds", f"tpc{partition_id}.timing_cmds", "TimingHwCmd", Direction.OUT)
                controllers_graph.add_endpoint(tmc_conf_gen.master_device_name+"_info", f"tpc{partition_id}.{tmc_conf_gen.master_device_name}_info", "JSON", Direction.IN, is_pubsub=True)
                
    for i,fanout_device_name in enumerate(FANOUT_DEVICES_NAMES):
        controllers_graph.add_endpoint("timing_cmds", f"tfc{i}.timing_cmds", "TimingHwCmd", Direction.OUT)

        controllers_graph.add_endpoint(fanout_device_name+"_info", f"tfc{i}.{fanout_device_name}_info", "JSON", Direction.IN, is_pubsub=True)
        thi_graph.add_endpoint( fanout_device_name+"_info", "thi."+fanout_device_name+"_info", "JSON", Direction.OUT, is_pubsub=True)

    if HSI_DEVICE_NAME:
        controllers_graph.add_endpoint("timing_cmds", f"hsic.timing_cmds", "TimingHwCmd", Direction.OUT)

        controllers_graph.add_endpoint(HSI_DEVICE_NAME+"_info", f"hsic.{HSI_DEVICE_NAME}_info", "JSON", Direction.IN, is_pubsub=True)
        thi_graph.add_endpoint( HSI_DEVICE_NAME+"_info", "thi."+HSI_DEVICE_NAME+"_info", "JSON", Direction.OUT, is_pubsub=True)
    
    
    thi_graph.add_endpoint("timing_cmds", "thi.timing_cmds", "TimingHwCmd", Direction.IN)
    #thi_graph.add_endpoint("timing_device_info", "thi.timing_device_info", "JSON", Direction.OUT, set(devices))

    thi_app = App(modulegraph=thi_graph, host=thi_conf_gen.host_thi)
    controllers_app = App(modulegraph=controllers_graph, host=tmc_conf_gen.host_tmc)

    controllers_app_name="ctrls"

    the_system = System()
    the_system.apps["thi"]=thi_app
    the_system.apps[controllers_app_name]=controllers_app
    
    ####################################################################
    # Application command data generation
    ####################################################################
    from daqconf.core.conf_utils import make_app_command_data
    # Arrange per-app command data into the format used by util.write_json_files()
    app_command_datas = {
        name : make_app_command_data(the_system, app, name, verbose=DEBUG, use_k8s=(boot.process_manager=='k8s'), use_connectivity_service=boot.use_connectivity_service)
        for name,app in the_system.apps.items()
    }

    # Make boot.json config
    from daqconf.core.conf_utils import make_system_command_datas, write_json_files
    system_command_datas = make_system_command_datas(
        boot,
        the_system,
        verbose=DEBUG
    )

    write_json_files(app_command_datas, system_command_datas, OUTPUT_PATH, verbose=DEBUG)

    console.log(f"Timing app config generated in {OUTPUT_PATH}")

    write_metadata_file(OUTPUT_PATH, "timing_app_confgen", config_file)

    console.log("Generating custom timing RC commands")

    data_dir = f"{OUTPUT_PATH}/data"

    for c,d in custom_cmds:
        cfg = {
            "apps": {controllers_app_name: f'data/{controllers_app_name}_{c}'}
        }
        with open(f"{OUTPUT_PATH}/{c}.json", 'w') as f:
            json.dump(cfg, f, indent=4, sort_keys=True)

        with open(f'{data_dir}/{controllers_app_name}_{c}.json', 'w') as f:
            json.dump(d.pod(), f, indent=4, sort_keys=True)

def split_string(ctx, param, value):
    if value is None:
        return []

    return value.split(',')

if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @generate_cli_from_schema('timinglibs/timing_app_confgen.jsonnet', 'app_confgen')
    @click.option('-r', '--run-number', default=333)
    
    @click.option('-p', '--partition-ids', default="0", callback=split_string)
    @click.option('--part-trig-mask', default=0xff)
    @click.option('--part-spill-gate', type=bool, default=True)
    @click.option('--part-rate-control', type=bool, default=True)

    @click.option('-f', '--fanout-devices-names', callback=split_string)
    @click.option('--fanout-clock-file', default="")

    @click.option('--hsi-clock-file', default="")
    @click.option('--hsi-random-rate', default=1.0)

    @click.option('-u', '--uhal-log-level', default="notice")
    @click.option('--debug', default=False, is_flag=True, help="Switch to get a lot of printout and dot files")
    @click.argument('json_dir', type=click.Path(), default='timing_app')
    def cli(config, run_number, partition_ids, part_trig_mask, part_spill_gate, part_rate_control,
        
        fanout_devices_names, fanout_clock_file,
        hsi_clock_file, hsi_random_rate,
        uhal_log_level, debug, json_dir):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        generate(
                    CONFIG = config,
                    RUN_NUMBER = run_number,
                    PARTITION_IDS = partition_ids,
                    FANOUT_DEVICES_NAMES = fanout_devices_names,
                    FANOUT_CLOCK_FILE = fanout_clock_file,
                    HSI_CLOCK_FILE = hsi_clock_file,
                    HSI_RANDOM_RATE=hsi_random_rate,
                    PART_TRIGGER_MASK=part_trig_mask,
                    PART_SPILL_GATE_ENABLED=part_spill_gate,
                    PART_RATE_CONTROL_ENABLED=part_rate_control,
                    UHAL_LOG_LEVEL = uhal_log_level,
                    OUTPUT_PATH = json_dir,
                    DEBUG = debug,
                )
    cli()
    
