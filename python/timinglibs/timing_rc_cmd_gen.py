from rich.console import Console
console = Console()

# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('timinglibs/timingcmd.jsonnet')
import dunedaq.timinglibs.timingcmd as tcmd

from appfwk.utils import acmd

import json
import math

def generate_timing_rc_cmds(
        MASTER_CONTROLLER_APP_NAME,
        MASTER_CONTROLLER_MOD_NAME,
        JSON_DIR,
        DEBUG=False,
    ):

    cmds = [
        ("master_send_fl_command",    acmd([ (MASTER_CONTROLLER_MOD_NAME, tcmd.TimingMasterSendFLCmdCmdPayload(
                                                                fl_cmd_id=0x1,
                                                                channel=0,
                                                                number_of_commands_to_send=1))])),
        ("master_set_endpoint_delay", acmd([ (MASTER_CONTROLLER_MOD_NAME, tcmd.TimingMasterSetEndpointDelayCmdPayload(
                                                                address=0,
                                                                coarse_delay=0,
                                                                fine_delay=0,
                                                                phase_delay=0,
                                                                measure_rtt=False,
                                                                control_sfp=False,
                                                                sfp_mux=-1))])),
        ]

    data_dir = f"{JSON_DIR}/data"

    for c,d in cmds:
        cfg = {
            "apps": {MASTER_CONTROLLER_APP_NAME: f'data/{MASTER_CONTROLLER_APP_NAME}_{c}'}
        }
        with open(f"{JSON_DIR}/{c}.json", 'w') as f:
            json.dump(cfg, f, indent=4, sort_keys=True)

        with open(f'{data_dir}/{MASTER_CONTROLLER_APP_NAME}_{c}.json', 'w') as f:
            json.dump(d.pod(), f, indent=4, sort_keys=True)

if __name__ == '__main__':
    # Add -h as default help option
    CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

    import click

    @click.command(context_settings=CONTEXT_SETTINGS)

    @click.option('--master-controller-app-name', default="")
    @click.option('--master-controller-mod-name', default="")

    @click.argument('json_dir', type=click.Path(), default="")
    def cli(
        master_controller_app_name, master_controller_mod_name, json_dir):

        generate_timing_rc_cmds(
                                MASTER_CONTROLLER_APP_NAME=master_controller_app_name,
                                MASTER_CONTROLLER_MOD_NAME=master_controller_mod_name,
                                JSON_DIR=json_dir,
                )
    cli()