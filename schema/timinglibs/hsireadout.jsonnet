local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.hsireadout";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {
    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    count : s.number("Count", "i4",
        doc="A count of not too many things"),

    str : s.string("Str", doc="A string field"),

    uhal_log_level : s.string("UHALLogLevel", pattern=moo.re.ident_only,
                    doc="Log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),

    init: s.record("InitParams", [
        s.field("qinfos", app.QueueInfos,
                doc="Information for a module to find its queue"),
        s.field("connections_file", self.str, "",
                doc="device connections file"),
        s.field("readout_period", self.uint_data, 1000,
                doc="Hardware device poll period [us]"),
        s.field("hsi_device_name", self.str, "",
                doc="Name of timing master device to be monitored"),
        s.field("uhal_log_level", self.uhal_log_level, "notice",
                doc="Log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),
    ], doc="HSIReadout init"),

};

s_app + moo.oschema.sort_select(cs)