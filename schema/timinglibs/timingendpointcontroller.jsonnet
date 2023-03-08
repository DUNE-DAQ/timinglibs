local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timingendpointcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {
	
    str : s.string("Str", doc="A string field"),
    
    size: s.number("Size", "u8",
        doc="A count of very many things"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    bool_data: s.boolean("BoolData", doc="A bool"),

    conf: s.record("ConfParams",[
        s.field("device", self.str, "",
                doc="String of managed device name"),
        s.field("hardware_state_recovery_enabled", self.bool_data, true,
            doc="control flag for hardware state recovery"),
        s.field("timing_session_name", self.str, "",
                doc="Name of managed device timing session"),
        s.field("endpoint_id", self.uint_data, optional=true,
            doc="ID of managed endpoint"),
        s.field("address", self.uint_data,
            doc="Endpoint address"),
        s.field("partition", self.uint_data,
            doc="Endpoint partition"),
    ], doc="Structure for payload of endpoint configure commands"),
};

s_app + moo.oschema.sort_select(cs)