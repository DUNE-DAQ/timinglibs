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
moo.otypes.load_types('timinglibs/hsireadout.jsonnet')
moo.otypes.load_types('trigger/timingtriggercandidatemaker.jsonnet')
moo.otypes.load_types('networkmanager/nwmgr.jsonnet')

# Import new types
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # Queue spec
import dunedaq.cmdlib.cmd as cmdlib # Command
import dunedaq.rcif.cmd as rcif # rcif
import dunedaq.networkmanager.nwmgr as nwmgr

import dunedaq.timinglibs.hsireadout as hsi
import dunedaq.trigger.timingtriggercandidatemaker as ttcm

from appfwk.utils import mcmd, mspec, mrccmd

import json
import math

def generate(
        PARTITION = "hsi_readout_test",
        RUN_NUMBER = 333, 
        CONNECTIONS_FILE="${TIMING_SHARE}/config/etc/connections.xml",
        READOUT_PERIOD = 1e3,
        HSI_DEVICE_NAME="BOREAS_FMC",
        TTCM_S1: int = 1,
        TTCM_S2: int = 2,
        UHAL_LOG_LEVEL="notice",
        OUTPUT_PATH=".",
    ):
    
    # network connection
    nw_specs = [nwmgr.Connection(name=PARTITION + ".hsievent",topics=[],  address="tcp://127.0.0.1:12344")]

    # Define modules and queues
    queue_bare_specs = [
            app.QueueSpec(inst="trigger_candidate_q", kind='FollySPSCQueue', capacity=2000),

        ]

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))

    mod_specs = [   
                    mspec("hsi", "HSIReadout", []),

                    mspec("ttcm", "TimingTriggerCandidateMaker", [
                                    app.QueueInfo(name="output", inst="trigger_candidate_q", dir="output"),
                                ]),
                ]

    init_specs = app.Init(queues=queue_specs, modules=mod_specs, nwconnections=nw_specs)

    jstr = json.dumps(init_specs.pod(), indent=4, sort_keys=True)
    print(jstr)

    initcmd = rcif.RCCommand(
        id=cmdlib.CmdId("init"),
        entry_state="NONE",
        exit_state="INITIAL",
        data=init_specs
    )

    mods = [
                ("hsi", hsi.ConfParams(
                        connections_file=CONNECTIONS_FILE,
                        readout_period=READOUT_PERIOD,
                        hsi_device_name=HSI_DEVICE_NAME,
                        uhal_log_level=UHAL_LOG_LEVEL,
                        hsievent_connection_name = f"{PARTITION}.hsievent",
                        )),
                
                ("ttcm", ttcm.Conf(
                        s1=ttcm.map_t(signal_type=TTCM_S1,
                                      time_before=100000,
                                      time_after=200000),
                        s2=ttcm.map_t(signal_type=TTCM_S2,
                                      time_before=100000,
                                      time_after=200000),
                        hsievent_connection_name = PARTITION+".hsievent",
                        )),
            ]

    confcmd = mrccmd("conf", "INITIAL", "CONFIGURED", mods)

    jstr = json.dumps(confcmd.pod(), indent=4, sort_keys=True)
    print(jstr)

    startpars = rcif.StartParams(run=1, disable_data_storage=False)

    startcmd = mrccmd("start", "CONFIGURED", "RUNNING", [
            ("hsi", None),
            ("ttcm", startpars),
        ])

    jstr = json.dumps(startcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStart\n\n", jstr)

    stopcmd = mrccmd("stop", "RUNNING", "CONFIGURED", [
            ("hsi", None),
            ("ttcm", None),
        ])

    jstr = json.dumps(stopcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nStop\n\n", jstr)


    scrapcmd = mcmd("scrap", [
            ("", None)
        ])

    jstr = json.dumps(scrapcmd.pod(), indent=4, sort_keys=True)
    print("="*80+"\nScrap\n\n", jstr)

    # Create a list of commands
    cmd_seq = [initcmd, confcmd, startcmd, stopcmd]

    # Print them as json (to be improved/moved out)
    jstr = json.dumps([c.pod() for c in cmd_seq], indent=4, sort_keys=True)
    return jstr
        
if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)
    @click.option('-p', '--partition', default="hsi_readout_test")
    @click.option('-r', '--run-number', default=333)
    @click.option('-c', '--connections-file', default="${TIMING_SHARE}/config/etc/connections.xml")
    @click.option('--readout-period', default=1e3)
    @click.option('-h', '--hsi-device-name', default="BOREAS_FMC")
    @click.option('--ttcm-s1', default=1, help="Timing trigger candidate maker accepted HSI signal ID 1")
    @click.option('--ttcm-s2', default=2, help="Timing trigger candidate maker accepted HSI signal ID 2")
    @click.option('-u', '--uhal-log-level', default="notice")
    @click.option('-o', '--output-path', type=click.Path(), default='.')
    @click.argument('json_file', type=click.Path(), default='hsi_readout_app.json')
    def cli(partition, run_number, connections_file, readout_period, hsi_device_name, ttcm_s1, ttcm_s2, uhal_log_level, output_path, json_file):
        """
          JSON_FILE: Input raw data file.
          JSON_FILE: Output json configuration file.
        """

        with open(json_file, 'w') as f:
            f.write(generate(
                    PARTITION=partition,
                    RUN_NUMBER = run_number,
                    CONNECTIONS_FILE=connections_file,
                    READOUT_PERIOD = readout_period,
                    HSI_DEVICE_NAME = hsi_device_name,
                    TTCM_S1 = ttcm_s1,
                    TTCM_S2 = ttcm_s2,
                    UHAL_LOG_LEVEL = uhal_log_level,
                    OUTPUT_PATH = output_path,
                ))

        print(f"'{json_file}' generation completed.")

    cli()
    