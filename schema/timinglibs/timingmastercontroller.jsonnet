local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timingmastercontroller";
local s = moo.oschema.schema(ns);

local stimingcmd = import "timinglibs/timingcmd.jsonnet";
local timingcmd = moo.oschema.hier(stimingcmd).dunedaq.timinglibs.timingcmd;

local cs = {

    str : s.string("Str", doc="A string field"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),
    
    bool_data: s.boolean("BoolData", doc="A bool"),
    
    fanout_mode: s.number("FanoutMode", "i4", 
        doc="Fanout mode"),

    conf: s.record("ConfParams", [
        s.field("device", self.str, "",
                doc="String of managed device name"),
        s.field("hardware_state_recovery_enabled", self.bool_data, true,
            doc="control flag for hardware state recovery"),
        s.field("timing_session_name", self.str, "",
                doc="Name of managed device timing session"),
        s.field("endpoint_scan_period", self.uint_data, 0,
                doc="Period between endpoint scans. 0 for disabled."),
        s.field("clock_config", self.str, "",
            doc="Path of clock config file"),
        s.field("soft", self.bool_data, false,
            doc="Soft reset"),
        s.field("fanout_mode", self.fanout_mode, -1, 
            doc="Fanout mode. -1: none, 0: fanout, 1: standalone"),
        s.field("monitored_endpoints", timingcmd.TimingEndpointLocations,
                doc="List of monitored endpoint addresses"),
    ], doc="TimingMasterController configuration"),

};

stimingcmd + moo.oschema.sort_select(cs)