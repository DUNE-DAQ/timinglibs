local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timinghardwaremanagerpdi";
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
    
    fanout_device_names_vector: s.sequence("FanoutDeviceNamesVector", self.str,
            doc="A vector of fanout device names"),
    
    init: s.record("ConfParams", [
        s.field("connections_file", self.str, "",
                doc="device connections file"),
        s.field("gather_interval", self.uint_data, 1e6,
                doc="Hardware device data gather interval [us]"),
        s.field("gather_interval_debug", self.uint_data, 10e6,
                doc="Hardware device data gather debug interval [us]"),
        s.field("monitored_device_name_master", self.str, "",
                doc="Name of timing master device to be monitored"),
        s.field("monitored_device_names_fanout", self.fanout_device_names_vector,
                doc="Names of timing fanout devices to be monitored"),
        s.field("monitored_device_name_endpoint", self.str, "",
                doc="Name of timing endpoint device to be monitored"),
        s.field("monitored_device_name_hsi", self.str, "",
                doc="Name of hsi device to be monitored"),
        s.field("device_info_connection", self.str, "",
            doc="String of managed hsi device info connection"),
        s.field("uhal_log_level", self.uhal_log_level, "notice",
                doc="Log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),
    ], doc="TimingHardwareManager PD-I conf parameters"),
};

s_app + moo.oschema.sort_select(cs)