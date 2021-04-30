import json
import os
import rich.traceback
from rich.console import Console
from os.path import exists, join

# Add -h as default help option
CONTEXT_SETTINGS = dict(help_option_names=['-h', '--help'])

console = Console()

def generate_boot( tremu_spec: dict, rudf_spec: dict, hsi_spec: dict) -> dict:
    """
    Generates boot informations

    :param      tremu_spec:  The tremu specifications
    :type       tremu_spec:  dict
    :param      rudf_spec:   The rudf specifications
    :type       rudf_spec:   dict

    :returns:   { description_of_the_return_value }
    :rtype:     dict
    """

    # TODO: think how to best handle this bit.
    # Who is responsible for this bit? Not minidaqapp?
    # Is this an appfwk configuration fragment?
    daq_app_specs = {
        "daq_application_ups" : {
            "comment": "Application profile based on a full dbt runtime environment",
            "env": {
               "DBT_AREA_ROOT": "getenv",
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_ID}_${APP_PORT}.json",
                "cd ${DBT_AREA_ROOT}",
                "source dbt-setup-env.sh",
                "dbt-setup-runtime-environment",
                "cd ${APP_WD}",
                "daq_application --name ${APP_ID} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        },
        "daq_application" : {
            "comment": "Application profile using  PATH variables (lower start time)",
            "env":{
                "CET_PLUGIN_PATH": "getenv",
                "DUNEDAQ_SHARE_PATH": "getenv",
                "LD_LIBRARY_PATH": "getenv",
                "PATH": "getenv",
                "DUNEDAQ_ERS_DEBUG_LEVEL": "getenv"
            },
            "cmd": [
                "CMD_FAC=rest://localhost:${APP_PORT}",
                "INFO_SVC=file://info_${APP_NAME}_${APP_PORT}.json",
                "cd ${APP_WD}",
                "daq_application --name ${APP_NAME} -c ${CMD_FAC} -i ${INFO_SVC}"
            ]
        }
    }

    boot = {
        "env": {
            "DUNEDAQ_ERS_VERBOSITY_LEVEL": "getenv",
            "DUNEDAQ_ERS_DEBUG_LEVEL": "getenv"
        },
        "apps": {
            rudf_spec['name']: {
                "exec": "daq_application",
                "host": "host_rudf",
                "port": rudf_spec["port"]
            },
            tremu_spec["name"]: {
                "exec": "daq_application",
                "host": "host_trgemu",
                "port": tremu_spec["port"]
            },
            hsi_spec["name"]: {
                "exec": "daq_application",
                "host": "host_hsi",
                "port": hsi_spec["port"]
            }
        },
        "hosts": {
            "host_rudf": rudf_spec["host"],
            "host_trgemu": tremu_spec["host"],
            "host_hsi": hsi_spec["host"]
        },
        "response_listener": {
            "port": 56789
        },
        "exec": daq_app_specs
    }

    console.log("Boot data")
    console.log(boot)
    return boot

import click

@click.command(context_settings=CONTEXT_SETTINGS)
@click.option('-n', '--number-of-data-producers', default=2)
@click.option('-e', '--emulator-mode', is_flag=True)
@click.option('-s', '--data-rate-slowdown-factor', default=1)
@click.option('-r', '--run-number', default=333)
@click.option('-t', '--trigger-rate-hz', default=1.0)
@click.option('-c', '--token-count', default=10)
@click.option('-d', '--data-file', type=click.Path(), default='./frames.bin')
@click.option('-o', '--output-path', type=click.Path(), default='.')
@click.option('--disable-data-storage', is_flag=True)
@click.option('-f', '--use-felix', is_flag=True)
@click.option('--clock-frequency', default=50e6)
@click.option('--hsi-event-period', default=1e9)
@click.option('--hsi-device-id', default=0)
@click.option('--mean-hsi-signal-multiplicity', default=1)
@click.option('--hsi-signal-emulation-mode', default=0)
@click.option('--enabled-hsi-signals', default=0b00000001)
@click.option('--host-rudf', default='localhost')
@click.option('--host-trgemu', default='localhost')
@click.option('--host-hsi', default='localhost')
@click.argument('json_dir', type=click.Path())
def cli(number_of_data_producers, emulator_mode, data_rate_slowdown_factor, run_number, trigger_rate_hz, token_count, data_file, output_path, disable_data_storage, use_felix, clock_frequency, hsi_event_period, hsi_device_id, mean_hsi_signal_multiplicity, hsi_signal_emulation_mode, enabled_hsi_signals, host_rudf, host_trgemu, host_hsi, json_dir):
    """
      JSON_DIR: Json file output folder
    """
    console.log("Loading rudf config generator")
    from . import rudf_gen
    
    console.log("Loading trg config generator")
    from . import trigger_gen
    
    console.log("Loading fake hsi config generator")
    from . import fake_hsi_gen
    
    console.log(f"Generating configs for hosts hsi={host_hsi} trg={host_trgemu} rudf={host_rudf}")

    if token_count > 0:
        df_token_count = 0
        trigemu_token_count = token_count
    else:
        df_token_count = -1 * token_count
        trigemu_token_count = 0

    network_endpoints={
        "trigdec" : "tcp://{host_trgemu}:12345",
        "triginh" : "tcp://{host_rudf}:12346",
        "timesync": "tcp://{host_rudf}:12347",
        "hsievent": "tcp://{host_hsi}:12348"
    }


    cmd_data_trg = trigger_gen.generate(
        network_endpoints,
        TRIGGER_RATE_HZ = trigger_rate_hz,
        OUTPUT_PATH = ".",
        TOKEN_COUNT = trigemu_token_count,
        CLOCK_SPEED_HZ = clock_frequency,
#        FORGET_DECISION_PROB = forget_decision_prob,
#        HOLD_DECISION_PROB = hold_decision_prob,
#        HOLD_MAX_SIZE = hold_max_size,
#        HOLD_MIN_SIZE = hold_min_size,
#        HOLD_MIN_MS = hold_min_ms,
#        RELEASE_RANDOMLY_PROB = release_randomly_prob
    )

    console.log("trg cmd data:", cmd_data_trg)


    cmd_data_hsi = fake_hsi_gen.generate(
        network_endpoints,
        RUN_NUMBER = run_number,
        CLOCK_SPEED_HZ = clock_frequency,
        HSI_EVENT_PERIOD_NS = hsi_event_period,
        HSI_DEVICE_ID = hsi_device_id,
        MEAN_SIGNAL_MULTIPLICITY = mean_hsi_signal_multiplicity,
        SIGNAL_EMULATION_MODE = hsi_signal_emulation_mode,
        ENABLED_SIGNALS =  enabled_hsi_signals,
    )

    console.log("hsi cmd data:", cmd_data_hsi)


    cmd_data_rudf = rudf_gen.generate(
        network_endpoints,
        NUMBER_OF_DATA_PRODUCERS = number_of_data_producers,
        EMULATOR_MODE = emulator_mode,
        DATA_RATE_SLOWDOWN_FACTOR = data_rate_slowdown_factor,
        RUN_NUMBER = run_number, 
        DATA_FILE = data_file,
        OUTPUT_PATH = output_path,
        DISABLE_OUTPUT = disable_data_storage,
        FLX_INPUT = use_felix,
        TOKEN_COUNT = df_token_count,
        CLOCK_SPEED_HZ = clock_frequency
    )
    console.log("rudf cmd data:", cmd_data_rudf)


    if exists(json_dir):
        raise RuntimeError(f"Directory {json_dir} already exists")

    data_dir = join(json_dir, 'data')
    os.makedirs(data_dir)

    app_trgemu="trgemu"
    app_hsi="fhsi"
    app_rudf="ruflx_df" if use_felix else "ruemu_df"

    jf_trigemu = join(data_dir, app_trgemu)
    jf_dfru = join(data_dir, app_rudf)
    jf_hsi = join(data_dir, app_hsi)

    cmd_set = ["init", "conf", "start", "stop", "pause", "resume", "scrap"]
    for app,data in ((app_hsi, cmd_data_hsi), (app_rudf, cmd_data_rudf), (app_trgemu, cmd_data_trg)):
        console.log(f"Generating {app} command data json files")
        for c in cmd_set:
            with open(f'{join(data_dir, app)}_{c}.json', 'w') as f:
                json.dump(data[c].pod(), f, indent=4, sort_keys=True)


    console.log(f"Generating top-level command json files")
    start_order = [app_rudf, app_hsi, app_trgemu]
    for c in cmd_set:
        with open(join(json_dir,f'{c}.json'), 'w') as f:
            cfg = {
                "apps": { app: f'data/{app}_{c}' for app in (app_hsi, app_rudf, app_trgemu) }
            }
            if c == 'start':
                cfg['order'] = start_order
            elif c == 'stop':
                cfg['order'] = start_order[::-1]
            elif c in ('resume', 'pause'):
                del cfg['apps'][app_rudf]
                del cfg['apps'][app_hsi]

            json.dump(cfg, f, indent=4, sort_keys=True)


    console.log(f"Generating boot json file")
    with open(join(json_dir,'boot.json'), 'w') as f:
        cfg = generate_boot(
            tremu_spec = {
                "name": app_trgemu,
                "host": host_trgemu,
                "port": 3335
            },
            rudf_spec = {
                "name": app_rudf,
                "host": host_rudf,
                "port": 3334
            },
            hsi_spec = {
                "name": app_hsi,
                "host": host_hsi,
                "port": 3333
            }
        )
        json.dump(cfg, f, indent=4, sort_keys=True)
    console.log(f"MDAapp config generated in {json_dir}")


if __name__ == '__main__':

    try:
        cli(show_default=True, standalone_mode=True)
    except Exception as e:
        console.print_exception()
