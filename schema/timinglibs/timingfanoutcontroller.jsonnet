local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timingfanoutcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    str : s.string("Str", doc="A string field"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    bool_data: s.boolean("BoolData", doc="A bool"),

    init: s.record("ConfParams", [
        s.field("device", self.str, "",
                doc="String of managed device name"),
        s.field("hardware_state_recovery_enabled", self.bool_data, true,
            doc="control flag for hardware state recovery"),
        s.field("timing_session_name", self.str, "",
                doc="Name of managed device timing session"),
        s.field("clock_config", self.str, "",
            doc="Path of clock config file"),
    ], doc="TimingFanoutController configuration"),

};

s_app + moo.oschema.sort_select(cs)