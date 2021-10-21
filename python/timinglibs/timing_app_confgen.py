from rich.console import Console
console = Console()

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')
moo.otypes.load_types('cmdlib/cmd.jsonnet')
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('timinglibs/timingcmd.jsonnet')
moo.otypes.load_types('timinglibs/timinghardwaremanagerpdi.jsonnet')
moo.otypes.load_types('timinglibs/timingmastercontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingfanoutcontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingpartitioncontroller.jsonnet')
moo.otypes.load_types('timinglibs/timingendpointcontroller.jsonnet')
moo.otypes.load_types('timinglibs/hsicontroller.jsonnet')

# Import new types
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # Queue spec
import dunedaq.cmdlib.cmd as cmdlib # Command
import dunedaq.rcif.cmd as rcif # rcif

import dunedaq.timinglibs.timingcmd as tcmd
import dunedaq.timinglibs.timinghardwaremanagerpdi as thi
import dunedaq.timinglibs.timingmastercontroller as tmc
import dunedaq.timinglibs.timingpartitioncontroller as tpc
import dunedaq.timinglibs.timingendpointcontroller as tec
import dunedaq.timinglibs.hsicontroller as hsi
import dunedaq.timinglibs.timingfanoutcontroller as tfc

from appfwk.utils import mcmd, mspec, mrccmd

import json
import math

CLOCK_SPEED_HZ=50000000

def generate(
        RUN_NUMBER = 333, 
        GATHER_INTERVAL = 1e6,
        GATHER_INTERVAL_DEBUG = 10e6,
        MASTER_DEVICE_NAME="",
        MASTER_CLOCK_FILE="",
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
        OUTPUT_PATH=".",
    ):
    
    # Define modules and queues
    queue_bare_specs = [
            app.QueueSpec(inst="hardware_commands", kind='StdDeQueue', capacity=100),
        ]

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))

    thi_init_data = thi.InitParams(
                                   qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_in", inst="hardware_commands", dir="input")]),
                                   connections_file="${TIMING_SHARE}/config/etc/connections.xml",
                                   gather_interval=GATHER_INTERVAL,
                                   gather_interval_debug=GATHER_INTERVAL_DEBUG,
                                   monitored_device_name_master=MASTER_DEVICE_NAME,
                                   monitored_device_names_fanout=FANOUT_DEVICES_NAMES,
                                   monitored_device_name_endpoint=ENDPOINT_DEVICE_NAME,
                                   monitored_device_name_hsi=HSI_DEVICE_NAME,
                                   uhal_log_level=UHAL_LOG_LEVEL
                                  )
    mod_specs = [
                    app.ModSpec(inst="thi", plugin="TimingHardwareManagerPDI", data=thi_init_data)
                ]

    if MASTER_DEVICE_NAME != "":
        master_controller_init_data = tmc.InitParams(
                                                     qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_out", inst="hardware_commands", dir="output")]),
                                                     device=MASTER_DEVICE_NAME,
                                                     )

        mod_specs.extend( [ app.ModSpec(inst="tmc0", plugin="TimingMasterController", data=master_controller_init_data) ] )

        tpc_mods=[]
        for partition_id in PARTITION_IDS:

            part_controller_init_data = tpc.InitParams(
                                                       qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_out", inst="hardware_commands", dir="output")]),
                                                       device=MASTER_DEVICE_NAME,
                                                       partition_id=partition_id,
                                                      )
            tpc_mod = app.ModSpec(inst="tpc{}".format(partition_id), plugin="TimingPartitionController", data=part_controller_init_data)
            tpc_mods.append(tpc_mod)

        mod_specs.extend( tpc_mods )

    for i,fanout_device_name in enumerate(FANOUT_DEVICES_NAMES):
        fanout_controller_init_data = tfc.InitParams(
                                                     qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_out", inst="hardware_commands", dir="output")]),
                                                     device=fanout_device_name,
                                                    )
        mod_specs.extend( [ app.ModSpec(inst="tfc{}".format(i), plugin="TimingFanoutController", data=fanout_controller_init_data) ] )

    if ENDPOINT_DEVICE_NAME != "":
        endpoint_controller_init_data = tec.InitParams(
                                                     qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_out", inst="hardware_commands", dir="output")]),
                                                     device=ENDPOINT_DEVICE_NAME,
                                                     )
        mod_specs.extend( [ app.ModSpec(inst="tec0", plugin="TimingEndpointController", data=endpoint_controller_init_data) ] )

    if HSI_DEVICE_NAME != "":
        hsi_controller_init_data = hsi.InitParams(
                                                  qinfos=app.QueueInfos([app.QueueInfo(name="hardware_commands_out", inst="hardware_commands", dir="output")]),
                                                  device=HSI_DEVICE_NAME,
                                                 )
        
        mod_specs.extend( [ app.ModSpec(inst="hsi0", plugin="HSIController", data=hsi_controller_init_data) ] )

    init_specs = app.Init(queues=queue_specs, modules=mod_specs)
    
    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = rcif.RCCommand(
        id=cmdlib.CmdId("init"),
        entry_state="NONE",
        exit_state="INITIAL",
        data=init_specs
    )

    ## conf command
    mods = []

    if MASTER_DEVICE_NAME != "":
#        mods.extend( [
#                        ("tmc0", tmc.ConfParams(
#                                    device=MASTER_DEVICE_NAME,
#                                 )),
#                     ] )
        for partition_id in PARTITION_IDS:
            mods.extend( [
                            ("tpc{}".format(partition_id), tpc.PartitionConfParams(
                                                            trigger_mask=PART_TRIGGER_MASK,
                                                            spill_gate_enabled=PART_SPILL_GATE_ENABLED,
                                                            rate_control_enabled=PART_RATE_CONTROL_ENABLED,
                                                        )),
                        ] )

#    for i,fanout_device_name in enumerate(FANOUT_DEVICES_NAMES):
#        mods.extend( [
#                        ("tfc{}".format(i), tfc.ConfParams(
#                                device=fanout_device_name,
#                                )),
#                     ] )


    if ENDPOINT_DEVICE_NAME != "":
        mods.extend( [
                        ("tec0", tec.ConfParams(
                                address=ENDPOINT_ADDRESS,
                                partition=ENDPOINT_PARTITION
                                )),
                     ] )

    trigger_interval_ticks=0
    if HSI_DEVICE_NAME != "":
        if HSI_RANDOM_RATE > 0:
            trigger_interval_ticks=math.floor((1/HSI_RANDOM_RATE) * CLOCK_SPEED_HZ)
        else:
            console.log('WARNING! Emulated trigger rate of 0 will not disable signal emulation in real HSI hardware! To disable emulated HSI triggers, use  option: "--hsi-source 0" or mask all signal bits', style="bold red")

        mods.extend( [
                        ("hsi0", hsi.ConfParams(
                                clock_frequency=CLOCK_SPEED_HZ,
                                trigger_interval_ticks=trigger_interval_ticks,
                                address=HSI_ENDPOINT_ADDRESS,
                                partition=HSI_ENDPOINT_PARTITION,
                                rising_edge_mask=HSI_RE_MASK,                   
                                falling_edge_mask=HSI_FE_MASK,
                                invert_edge_mask=HSI_INV_MASK,
                                data_source=HSI_SOURCE,
                                )),
                     ] )

    confcmd = mrccmd("conf", "INITIAL", "CONFIGURED", mods)

    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = rcif.StartParams(run=RUN_NUMBER, trigger_interval_ticks = trigger_interval_ticks)
    startcmd = mrccmd("start", "CONFIGURED", "RUNNING", [
            ("thi", startpars),
            ("tmc.*", startpars),
            ("tpc.*", startpars),
            ("tfc.*", startpars),
            ("tec.*", startpars),
            ("hsi.*", startpars),
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)

    stopcmd = mrccmd("stop", "RUNNING", "CONFIGURED", [
            ("thi", None),
            ("tmc.*", None),
            ("tpc.*", None),
            ("tfc.*", None),
            ("tec.*", None),
            ("hsi.*", None),
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)


    scrapcmd = mcmd("scrap", [
            ("thi", None),
            ("tmc.*", None),
            ("tpc.*", None),
            ("tfc.*", None),
            ("tec.*", None),
            ("hsi.*", None),
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    ## timing specific commands

    # master commands
    master_io_reset_cmd = mcmd("master_io_reset", [
            ("tmc.*", tcmd.IOResetCmdPayload(
                      clock_config=MASTER_CLOCK_FILE,
                      soft=False
                      )),
        ])
    jstr = json.dumps(master_io_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nMaster IO reset\n\n", jstr)


    master_set_timestamp_cmd = mcmd("master_set_timestamp", [
            ("tmc.*", None),
        ])
    jstr = json.dumps(master_set_timestamp_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nMaster set timestamp\n\n", jstr)


    master_print_status_cmd = mcmd("master_print_status", [
            ("tmc.*", None),
        ])
    jstr = json.dumps(master_print_status_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nMaster print status\n\n", jstr)


    # partition commands
    partition_configure_cmd = mcmd("partition_configure", [
            ("tpc.*", tpc.PartitionConfParams(
                      trigger_mask=PART_TRIGGER_MASK,
                      spill_gate_enabled=PART_SPILL_GATE_ENABLED,
                      rate_control_enabled=PART_RATE_CONTROL_ENABLED,
                      )),
        ])
    jstr = json.dumps(partition_configure_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition configure\n\n", jstr)


    partition_enable_cmd = mcmd("partition_enable", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_enable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition enable\n\n", jstr)


    partition_disable_cmd = mcmd("partition_disable", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_disable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition disable\n\n", jstr)


    partition_start_cmd = mcmd("partition_start", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_start_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition start\n\n", jstr)


    partition_stop_cmd = mcmd("partition_stop", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_stop_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition stop\n\n", jstr)


    partition_enable_triggers_cmd = mcmd("partition_enable_triggers", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_enable_triggers_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition enable triggers\n\n", jstr)


    partition_disable_triggers_cmd = mcmd("partition_disable_triggers", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_disable_triggers_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition disable triggers\n\n", jstr)


    partition_print_status_cmd = mcmd("partition_print_status", [
            ("tpc.*", None),
        ])
    jstr = json.dumps(partition_print_status_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nPartition print status\n\n", jstr)

    # fanout commands
    fanout_io_reset_cmd = mcmd("fanout_io_reset", [
            ("tfc.*", tcmd.IOResetCmdPayload(
                      clock_config=FANOUT_CLOCK_FILE,
                      soft=False
                      )),
        ])
    jstr = json.dumps(fanout_io_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nFanout IO reset\n\n", jstr)


    fanout_print_status_cmd = mcmd("fanout_print_status", [
            ("tfc.*", None),
        ])
    jstr = json.dumps(fanout_print_status_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nFanout print status\n\n", jstr)

    # hsi commands
    hsi_io_reset_cmd = mcmd("hsi_io_reset", [
            ("hsi.*", tcmd.IOResetCmdPayload(
                      clock_config=HSI_CLOCK_FILE,
                      soft=False
                      )),
        ])
    jstr = json.dumps(hsi_io_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI IO reset\n\n", jstr)


    hsi_endpoint_enable_cmd = mcmd("hsi_endpoint_enable", [
            ("hsi.*", tcmd.TimingEndpointConfigureCmdPayload(
                      address=HSI_ENDPOINT_ADDRESS,
                      partition=HSI_ENDPOINT_PARTITION
                      )),
        ])
    jstr = json.dumps(hsi_endpoint_enable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI endpoint enable\n\n", jstr)


    hsi_endpoint_disable_cmd = mcmd("hsi_endpoint_disable", [
            ("hsi.*", None),
        ])
    jstr = json.dumps(hsi_endpoint_disable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI endpoint disable\n\n", jstr)


    hsi_endpoint_reset_cmd = mcmd("hsi_endpoint_reset", [
            ("hsi.*", tcmd.TimingEndpointConfigureCmdPayload(
                      address=HSI_ENDPOINT_ADDRESS,
                      partition=HSI_ENDPOINT_PARTITION
                      )),
        ])
    jstr = json.dumps(hsi_endpoint_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI endpoint reset\n\n", jstr)


    hsi_reset_cmd = mcmd("hsi_reset", [
            ("hsi.*", None),
        ])
    jstr = json.dumps(hsi_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI reset\n\n", jstr)


    hsi_configure_cmd = mcmd("hsi_configure", [
            ("hsi.*", tcmd.HSIConfigureCmdPayload(
                      rising_edge_mask=HSI_RE_MASK,                   
                      falling_edge_mask=HSI_FE_MASK,
                      invert_edge_mask=HSI_INV_MASK,
                      data_source=HSI_SOURCE,
                      random_rate=HSI_RANDOM_RATE
                      )),
        ])
    jstr = json.dumps(hsi_configure_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI configure\n\n", jstr)

    hsi_start_cmd = mcmd("hsi_start", [
            ("hsi.*", None),
        ])
    jstr = json.dumps(hsi_start_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI start\n\n", jstr)

    hsi_stop_cmd = mcmd("hsi_stop", [
            ("hsi.*", None),
        ])
    jstr = json.dumps(hsi_stop_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI stop\n\n", jstr)


    hsi_print_status_cmd = mcmd("hsi_print_status", [
            ("hsi.*", None),
        ])
    jstr = json.dumps(hsi_print_status_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nHSI print status\n\n", jstr)



    # endpoint commands
    endpoint_io_reset_cmd = mcmd("endpoint_io_reset", [
            ("tec.*", tcmd.IOResetCmdPayload(
                      clock_config=ENDPOINT_CLOCK_FILE,
                      soft=False
                      )),
        ])
    jstr = json.dumps(endpoint_io_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nEndpoint IO reset\n\n", jstr)


    endpoint_enable_cmd = mcmd("endpoint_enable", [
            ("tec.*", tcmd.TimingEndpointConfigureCmdPayload(
                      address=ENDPOINT_ADDRESS,
                      partition=ENDPOINT_PARTITION
                      )),
        ])
    jstr = json.dumps(endpoint_enable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nEndpoint enable\n\n", jstr)


    endpoint_disable_cmd = mcmd("endpoint_disable", [
            ("tec.*", None),
        ])
    jstr = json.dumps(endpoint_disable_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nEndpoint disable\n\n", jstr)


    endpoint_reset_cmd = mcmd("endpoint_reset", [
            ("tec.*", tcmd.TimingEndpointConfigureCmdPayload(
                      address=ENDPOINT_ADDRESS,
                      partition=ENDPOINT_PARTITION
                      )),
        ])
    jstr = json.dumps(endpoint_reset_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nEndpoint reset\n\n", jstr)


    endpoint_print_status_cmd = mcmd("endpoint_print_status", [
            ("tec.*", None),
        ])
    jstr = json.dumps(endpoint_print_status_cmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nEndpoint print status\n\n", jstr)
    #####

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd]

    if MASTER_DEVICE_NAME != "":
        cmd_seq.extend( [
                        master_io_reset_cmd, master_set_timestamp_cmd, master_print_status_cmd,
                        partition_configure_cmd, partition_enable_cmd, partition_disable_cmd, 
                        partition_start_cmd, partition_stop_cmd, 
                        partition_enable_triggers_cmd, partition_disable_triggers_cmd, 
                        partition_print_status_cmd
                        ] )
    
    if len(FANOUT_DEVICES_NAMES) != 0:
        cmd_seq.extend( [
                        fanout_io_reset_cmd, fanout_print_status_cmd,
                        ] )

    if ENDPOINT_DEVICE_NAME != "":
        cmd_seq.extend( [
                        endpoint_io_reset_cmd, 
                        endpoint_enable_cmd, endpoint_disable_cmd, 
                        endpoint_reset_cmd, endpoint_print_status_cmd
                        ] )

    if HSI_DEVICE_NAME != "":
        cmd_seq.extend( [
                        hsi_io_reset_cmd,
                        hsi_endpoint_enable_cmd,
                        hsi_endpoint_disable_cmd,
                        hsi_endpoint_reset_cmd,
                        hsi_reset_cmd,
                        hsi_configure_cmd,
                        hsi_start_cmd,
                        hsi_stop_cmd,
                        hsi_print_status_cmd,
                        ] )

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr

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
    @click.option('-g', '--gather-interval', default=1e6)
    @click.option('-d', '--gather-interval-debug', default=10e6)

    @click.option('-m', '--master-device-name', default="")
    @click.option('--master-clock-file', default="")
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
    @click.argument('json_file', type=click.Path(), default='timing_app.json')
    def cli(run_number, gather_interval, gather_interval_debug, 

        master_device_name, master_clock_file, partition_ids,
        fanout_devices_names, fanout_clock_file,
        endpoint_device_name, endpoint_clock_file, endpoint_address, endpoint_partition,
        hsi_device_name, hsi_clock_file, hsi_endpoint_address, hsi_endpoint_partition, hsi_re_mask, hsi_fe_mask, hsi_inv_mask, hsi_source, hsi_random_rate,
        part_trig_mask, part_spill_gate, part_rate_control,
        uhal_log_level, output_path, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    RUN_NUMBER = run_number, 
                    GATHER_INTERVAL = gather_interval,
                    GATHER_INTERVAL_DEBUG = gather_interval_debug,
                    MASTER_DEVICE_NAME = master_device_name,
                    MASTER_CLOCK_FILE = master_clock_file,
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
                    OUTPUT_PATH = output_path,
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    