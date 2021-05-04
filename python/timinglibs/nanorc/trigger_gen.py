# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')

moo.otypes.load_types('trigger/intervaltccreator.jsonnet')
moo.otypes.load_types('trigger/moduleleveltrigger.jsonnet')
moo.otypes.load_types('trigger/fakedataflow.jsonnet')
moo.otypes.load_types('trigger/timingtriggercandidatemaker.jsonnet')

moo.otypes.load_types('nwqueueadapters/queuetonetwork.jsonnet')
moo.otypes.load_types('nwqueueadapters/networktoqueue.jsonnet')
moo.otypes.load_types('nwqueueadapters/networkobjectreceiver.jsonnet')
moo.otypes.load_types('nwqueueadapters/networkobjectsender.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.intervaltccreator as itcc
import dunedaq.trigger.moduleleveltrigger as mlt
import dunedaq.trigger.fakedataflow as fdf
import dunedaq.trigger.timingtriggercandidatemaker as ttcm

import dunedaq.nwqueueadapters.networktoqueue as ntoq
import dunedaq.nwqueueadapters.queuetonetwork as qton
import dunedaq.nwqueueadapters.networkobjectreceiver as nor
import dunedaq.nwqueueadapters.networkobjectsender as nos

from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
from pprint import pprint


#===============================================================================
def acmd(mods: list) -> cmd.CmdObj:
    """ 
    Helper function to create appfwk's Commands addressed to modules.
        
    :param      cmdid:  The coommand id
    :type       cmdid:  str
    :param      mods:   List of module name/data structures 
    :type       mods:   list
    
    :returns:   A constructed Command object
    :rtype:     dunedaq.appfwk.cmd.Command
    """
    return cmd.CmdObj(
        modules=cmd.AddressedCmds(
            cmd.AddressedCmd(match=m, data=o)
            for m,o in mods
        )
    )

#===============================================================================
def generate(
        NETWORK_ENDPOINTS: list,
        NUMBER_OF_DATA_PRODUCERS: int = 2,
        TRIGGER_RATE_HZ: float = 1.0,
        OUTPUT_PATH: str = ".",
        TOKEN_COUNT: int = 10,
        CLOCK_SPEED_HZ: int = 50000000,
        FORGET_DECISION_PROB: float = 0.0,
        HOLD_DECISION_PROB: float = 0.0,
        HOLD_MAX_SIZE: int = 0,
        HOLD_MIN_SIZE: int = 0,
        HOLD_MIN_MS: int = 0,
        RELEASE_RANDOMLY_PROB: float = 0.0
):
    """
    { item_description }
    """
    cmd_data = {}

    # Derived parameters
    TRIGGER_INTERVAL_NS = math.floor((1e9/TRIGGER_RATE_HZ))

    # Define modules and queues
    queue_bare_specs = [
        app.QueueSpec(inst="hsievent_from_netq", kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst="token_from_netq", kind='FollySPSCQueue', capacity=2000),        
        app.QueueSpec(inst="trigger_decision_to_netq", kind='FollySPSCQueue', capacity=2000),
        app.QueueSpec(inst="trigger_candidate_q", kind='FollySPSCQueue', capacity=2000),
    ]

    # Only needed to reproduce the same order as when using jsonnet
    queue_specs = app.QueueSpecs(sorted(queue_bare_specs, key=lambda x: x.inst))

    mod_specs = [

        mspec("ntoq_hsievent", "NetworkToQueue", [
                        app.QueueInfo(name="output", inst="hsievent_from_netq", dir="output")
                    ]),

        mspec("ntoq_token", "NetworkToQueue", [
                        app.QueueInfo(name="output", inst="token_from_netq", dir="output")
                    ]),

        mspec("qton_trigdec", "QueueToNetwork", [
                        app.QueueInfo(name="input", inst="trigger_decision_to_netq", dir="input")
                    ]),

        mspec("mlt", "ModuleLevelTrigger", [
            app.QueueInfo(name="token_source", inst="token_from_netq", dir="input"),
            app.QueueInfo(name="trigger_decision_sink", inst="trigger_decision_to_netq", dir="output"),
            app.QueueInfo(name="trigger_candidate_source", inst="trigger_candidate_q", dir="output"),
        ]),

        mspec("ttcm", "TimingTriggerCandidateMaker", [
            app.QueueInfo(name="input", inst="hsievent_from_netq", dir="input"),
            app.QueueInfo(name="output", inst="trigger_candidate_q", dir="output"),
        ]),


    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    cmd_data['conf'] = acmd([
        ("mlt", mlt.ConfParams(
            links=[idx for idx in range(NUMBER_OF_DATA_PRODUCERS)],
            initial_token_count=TOKEN_COUNT                    
        )),
        
        ("ttcm", ttcm.Conf(
        )),

        ("ntoq_hsievent", ntoq.Conf(msg_type="dunedaq::dfmessages::HSIEvent",
                                           msg_module_name="HSIEventNQ",
                                           receiver_config=nor.Conf(ipm_plugin_type="ZmqReceiver",
                                                                    address=NETWORK_ENDPOINTS["hsievent"])
                                           )
                ),
        ("ntoq_token", ntoq.Conf(msg_type="dunedaq::dfmessages::TriggerDecisionToken",
                                           msg_module_name="TriggerDecisionTokenNQ",
                                           receiver_config=nor.Conf(ipm_plugin_type="ZmqReceiver",
                                                                    address=NETWORK_ENDPOINTS["triginh"])
                                           )
                ),
        ("qton_trigdec", qton.Conf(msg_type="dunedaq::dfmessages::TriggerDecision",
                                           msg_module_name="TriggerDecisionNQ",
                                           sender_config=nos.Conf(ipm_plugin_type="ZmqSender",
                                                                    address=NETWORK_ENDPOINTS["trigdec"])
                                           )
                ),
    ])

    startpars = rccmd.StartParams(run=1, disable_data_storage=False)
    cmd_data['start'] = acmd([
        ("mlt", startpars),
        ("ttcm", startpars),
        ("ntoq_hsievent", startpars),
        ("ntoq_token", startpars),
        ("qton_trigdec", startpars),
    ])

    cmd_data['stop'] = acmd([
        ("fdf", None),
        ("mlt", None),
        ("ttcm", None),
        ("ntoq_hsievent", None),
        ("ntoq_token", startpars),
        ("qton_trigdec", startpars),
    ])

    cmd_data['pause'] = acmd([
        ("", None)
    ])

    resumepars = rccmd.ResumeParams(trigger_interval_ticks=50000000)
    cmd_data['resume'] = acmd([
        ("mlt", resumepars)
    ])

    cmd_data['scrap'] = acmd([
        ("", None)
    ])

    return cmd_data
