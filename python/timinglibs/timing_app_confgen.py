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
moo.otypes.load_types('timinglibs/hsicontroller.jsonnet')

# Import new types
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.timinglibs.timingcmd as tcmd
import dunedaq.timinglibs.timinghardwaremanagerpdi as thi
import dunedaq.timinglibs.timingmastercontroller as tmc
import dunedaq.timinglibs.timingpartitioncontroller as tpc
import dunedaq.timinglibs.timingendpointcontroller as tec
import dunedaq.timinglibs.hsicontroller as hsi
import dunedaq.timinglibs.timingfanoutcontroller as tfc

from appfwk.utils import acmd
from daqconf.core.app import App, ModuleGraph
from daqconf.core.daqmodule import DAQModule
from daqconf.core.conf_utils import Direction, AppConnection
from daqconf.core.system import System
from daqconf.core.metadata import write_metadata_file

import json
import math

CLOCK_SPEED_HZ=50000000

def generate(
        RUN_NUMBER = 333, 
        CONNECTIONS_FILE="${TIMING_SHARE}/config/etc/connections.xml",
        GATHER_INTERVAL = 1e6,
        GATHER_INTERVAL_DEBUG = 10e6,
        MASTER_DEVICE_NAME="",
        MASTER_SEND_DELAYS_PERIOD=0,
        MASTER_CLOCK_FILE="",
        MASTER_CLOCK_MODE=-1,
        PARTITION_IDS=[],
        FANOUT_DEVICES_NAMES=[],
        FANOUT_CLOCK_FILE="",
        ENDPOINT_DEVICE_NAME="",
        ENDPOINT_CLOCK_FILE="",
        ENDPOINT_ADDRESS=0,
        ENDPOINT_PARTITION=0,
        HSI_DEVICE_NAME="",
        HSI_ENDPOINT_ADDRESS=0,
        HSI_ENDPOINT_PARTITION=0,
        HSI_CLOCK_FILE="",
        HSI_RE_MASK=0x0,
        HSI_FE_MASK=0x0,
        HSI_INV_MASK=0x0,
        HSI_RANDOM_RATE=1.0,
        HSI_SOURCE=0x0,
        PART_TRIGGER_MASK=0xff,
        PART_SPILL_GATE_ENABLED=True,
        PART_RATE_CONTROL_ENABLED=True,
        UHAL_LOG_LEVEL="notice",
        OUTPUT_PATH="./timing_app",
    ):
    
    # Define modules and queues
    
    thi_modules = [ 
                DAQModule( name="thi",
                                plugin="TimingHardwareManagerPDI",
                                conf= thi.ConfParams(connections_file=CONNECTIONS_FILE,
                                                       gather_interval=GATHER_INTERVAL,
                                                       gather_interval_debug=GATHER_INTERVAL_DEBUG,
                                                       monitored_device_name_master=MASTER_DEVICE_NAME,
                                                       monitored_device_names_fanout=FANOUT_DEVICES_NAMES,
                                                       monitored_device_name_endpoint=ENDPOINT_DEVICE_NAME,
                                                       monitored_device_name_hsi=HSI_DEVICE_NAME,
                                                       uhal_log_level=UHAL_LOG_LEVEL)),
                ]
    
    controller_modules = []
    custom_cmds=[]

    ## master and partition controllers
    master_controller_mod_name="tmc"
    
    if MASTER_DEVICE_NAME != "":
        controller_modules.extend( [DAQModule(name = master_controller_mod_name,
                    plugin = "TimingMasterController",
                    conf = tmc.ConfParams(
                                        device=MASTER_DEVICE_NAME,
                                        send_endpoint_delays_period=0,
                                        clock_config=MASTER_CLOCK_FILE,
                                        fanout_mode=MASTER_CLOCK_MODE,
                                        ))])


        custom_cmds.extend( [
                                ("master_configure", acmd([("tmc", tmc.ConfParams(
                                            device=MASTER_DEVICE_NAME,
                                            send_endpoint_delays_period=MASTER_SEND_DELAYS_PERIOD,
                                            clock_config=MASTER_CLOCK_FILE,
                                            fanout_mode=MASTER_CLOCK_MODE,
                                                                ))])),
                                ("master_io_reset", acmd([("tmc", tcmd.IOResetCmdPayload(
                                                  clock_config=MASTER_CLOCK_FILE,
                                                  fanout_mode=MASTER_CLOCK_MODE,
                                                  soft=False
                                                  ))])),
                                ("master_set_timestamp", acmd([("tmc",None)])),
                                ("master_print_status",  acmd([("tmc",None)])),
                            ] )

        ###
        tpc_mods=[]
        for partition_id in PARTITION_IDS:

            tpc_mods.append( DAQModule(name = f"tpc{partition_id}",
                         plugin = "TimingPartitionController",
                         conf = tpc.PartitionConfParams(
                                             device=MASTER_DEVICE_NAME,
                                             partition_id=partition_id,
                                             trigger_mask=PART_TRIGGER_MASK,
                                             spill_gate_enabled=PART_SPILL_GATE_ENABLED,
                                             rate_control_enabled=PART_RATE_CONTROL_ENABLED,
                                             )))
        custom_cmds.extend( [
                                ("partition_configure", acmd([("tpc*", tpc.PartitionConfParams(
                                                                device=MASTER_DEVICE_NAME,
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
                                                           clock_config=ENDPOINT_CLOCK_FILE,
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
        
        startpars = rccmd.StartParams(run=RUN_NUMBER, trigger_interval_ticks = trigger_interval_ticks)
        resumepars = rccmd.ResumeParams(trigger_interval_ticks = trigger_interval_ticks)

        controller_modules.extend( [ DAQModule(name="hsic",
                                plugin = "HSIController",
                                conf = hsi.ConfParams( device=HSI_DEVICE_NAME,
                                                        clock_frequency=CLOCK_SPEED_HZ,
                                                        trigger_interval_ticks=trigger_interval_ticks,
                                                        address=HSI_ENDPOINT_ADDRESS,
                                                        partition=HSI_ENDPOINT_PARTITION,
                                                        rising_edge_mask=HSI_RE_MASK,
                                                        falling_edge_mask=HSI_FE_MASK,
                                                        invert_edge_mask=HSI_INV_MASK,
                                                        data_source=HSI_SOURCE),
                                extra_commands = {"start": startpars,
                                                  "resume": resumepars}), ] )
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
                                                                          address=HSI_ENDPOINT_ADDRESS,
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
    debug=False
    partition_name="timing"
    THI_HOST="it064574.users.bris.ac.uk"
    CONTROLLER_HOST="it064574.users.bris.ac.uk"

    devices=[]

    controllers_graph = ModuleGraph(controller_modules)
    if ENDPOINT_DEVICE_NAME:
        devices.append(ENDPOINT_DEVICE_NAME)
        controllers_graph.add_endpoint("timing_cmds", "tec0.timing_cmds", Direction.OUT)
        controllers_graph.add_endpoint("timing_device_info", "tec0.timing_device_info", Direction.IN, [ENDPOINT_DEVICE_NAME])
    
    if MASTER_DEVICE_NAME:
        devices.append(MASTER_DEVICE_NAME)
        controllers_graph.add_endpoint("timing_cmds", f"{master_controller_mod_name}.timing_cmds", Direction.OUT)
        controllers_graph.add_endpoint("timing_device_info", f"{master_controller_mod_name}.timing_device_info", Direction.IN, [MASTER_DEVICE_NAME])

    for i,fanout_device_name in enumerate(FANOUT_DEVICES_NAMES):

        devices.append(fanout_device_name)
        controllers_graph.add_endpoint("timing_cmds", f"tfc{i}.timing_cmds", Direction.OUT)
        controllers_graph.add_endpoint("timing_device_info", f"tfc{i}.timing_device_info", Direction.IN, [fanout_device_name])

    thi_graph = ModuleGraph(thi_modules)
    thi_graph.add_endpoint("timing_cmds", "thi.timing_cmds", Direction.IN)
    thi_graph.add_endpoint("timing_device_info", "thi.timing_device_info", Direction.OUT, devices)

    thi_app = App(modulegraph=thi_graph, host=THI_HOST)
    controllers_app = App(modulegraph=controllers_graph, host=CONTROLLER_HOST)

    controllers_app_name="ctrls"

    the_system = System()
    the_system.apps["thi"]=thi_app
    the_system.apps[controllers_app_name]=controllers_app

    ers_settings=dict()

    use_kafka = False
    disable_trace=False
    info_svc_uri="file://info_${APP_NAME}_${APP_PORT}.json"
    
    ers_settings["INFO"] =    "erstrace,throttle,lstdout"
    ers_settings["WARNING"] = "erstrace,throttle,lstdout"
    ers_settings["ERROR"] =   "erstrace,throttle,lstdout"
    ers_settings["FATAL"] =   "erstrace,lstdout"
    
    ####################################################################
    # Application command data generation
    ####################################################################
    from daqconf.core.conf_utils import make_app_command_data
    # Arrange per-app command data into the format used by util.write_json_files()
    app_command_datas = {
        name : make_app_command_data(the_system, app, name, verbose=debug)
        for name,app in the_system.apps.items()
    }

    # Make boot.json config
    from daqconf.core.conf_utils import make_system_command_datas,generate_boot, write_json_files
    system_command_datas = make_system_command_datas(the_system, verbose=debug)
    # Override the default boot.json with the one from minidaqapp
    boot = generate_boot(the_system.apps, ers_settings=ers_settings, info_svc_uri=info_svc_uri,
                              disable_trace=disable_trace, use_kafka=use_kafka)

    system_command_datas['boot'] = boot

    write_json_files(app_command_datas, system_command_datas, OUTPUT_PATH, verbose=debug)

    console.log(f"timing app config generated in {OUTPUT_PATH}")
    
    write_metadata_file(OUTPUT_PATH, "timing_app_confgen")

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
    @click.option('-r', '--run-number', default=333)
    @click.option('-c', '--connections-file', default="${TIMING_SHARE}/config/etc/connections.xml", help='Path to timing hardware connections file')

    @click.option('-g', '--gather-interval', default=1e6)
    @click.option('-d', '--gather-interval-debug', default=10e6)

    @click.option('-m', '--master-device-name', default="")
    @click.option('--master-clock-file', default="")
    @click.option('--master-clock-mode', default=-1)
    @click.option('--master-send-delays-period', default=0, help="Master controller continuously send delays period [ms] (to all endpoints). 0 for disable.")
    @click.option('-p', '--partition-ids', default="0", callback=split_string)

    @click.option('-f', '--fanout-devices-names', callback=split_string)
    @click.option('--fanout-clock-file', default="")

    @click.option('-e', '--endpoint-device-name', default="")
    @click.option('--endpoint-clock-file', default="")
    @click.option('--endpoint-address', default=0)
    @click.option('--endpoint-partition', default=0)

    @click.option('-h', '--hsi-device-name', default="")
    @click.option('--hsi-clock-file', default="")
    @click.option('--hsi-endpoint-address', default=0)
    @click.option('--hsi-endpoint-partition', default=0)

    @click.option('--hsi-re-mask', default=0x0)
    @click.option('--hsi-fe-mask', default=0x0)
    @click.option('--hsi-inv-mask', default=0x0)
    @click.option('--hsi-source', default=0x0)
    @click.option('--hsi-random-rate', default=1.0)
    
    @click.option('--part-trig-mask', default=0xff)
    @click.option('--part-spill-gate', type=bool, default=True)
    @click.option('--part-rate-control', type=bool, default=True)

    @click.option('-u', '--uhal-log-level', default="notice")
    @click.option('-o', '--output-path', type=click.Path(), default='.')
    @click.argument('json_dir', type=click.Path(), default='timing_app')
    def cli(run_number, connections_file, gather_interval, gather_interval_debug, 

        master_device_name, master_clock_file, master_clock_mode, master_send_delays_period, partition_ids,
        fanout_devices_names, fanout_clock_file,
        endpoint_device_name, endpoint_clock_file, endpoint_address, endpoint_partition,
        hsi_device_name, hsi_clock_file, hsi_endpoint_address, hsi_endpoint_partition, hsi_re_mask, hsi_fe_mask, hsi_inv_mask, hsi_source, hsi_random_rate,
        part_trig_mask, part_spill_gate, part_rate_control,
        uhal_log_level, output_path, json_dir):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        generate(
                    RUN_NUMBER = run_number, 
                    CONNECTIONS_FILE=connections_file,
                    GATHER_INTERVAL = gather_interval,
                    GATHER_INTERVAL_DEBUG = gather_interval_debug,
                    MASTER_DEVICE_NAME = master_device_name,
                    MASTER_SEND_DELAYS_PERIOD=master_send_delays_period,
                    MASTER_CLOCK_FILE = master_clock_file,
                    MASTER_CLOCK_MODE = master_clock_mode,
                    PARTITION_IDS = partition_ids,
                    FANOUT_DEVICES_NAMES = fanout_devices_names,
                    FANOUT_CLOCK_FILE = fanout_clock_file,
                    ENDPOINT_DEVICE_NAME = endpoint_device_name,
                    ENDPOINT_CLOCK_FILE = endpoint_clock_file,
                    ENDPOINT_ADDRESS = endpoint_address,
                    ENDPOINT_PARTITION = endpoint_partition,
                    HSI_DEVICE_NAME = hsi_device_name,
                    HSI_CLOCK_FILE = hsi_clock_file,
                    HSI_ENDPOINT_ADDRESS = hsi_endpoint_address,
                    HSI_ENDPOINT_PARTITION = hsi_endpoint_partition,
                    HSI_RE_MASK=hsi_re_mask,
                    HSI_FE_MASK=hsi_fe_mask,
                    HSI_INV_MASK=hsi_inv_mask,
                    HSI_SOURCE=hsi_source,
                    HSI_RANDOM_RATE=hsi_random_rate,
                    PART_TRIGGER_MASK=part_trig_mask,
                    PART_SPILL_GATE_ENABLED=part_spill_gate,
                    PART_RATE_CONTROL_ENABLED=part_rate_control,
                    UHAL_LOG_LEVEL = uhal_log_level,
                    OUTPUT_PATH = json_dir,
                )
    cli()
    