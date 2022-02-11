local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timingmastercontroller";
local s = moo.oschema.schema(ns);
 
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
        s.field("clock_config", self.str, "",
            doc="Path of clock config file"),
        s.field("soft", self.bool_data, false,
            doc="Soft reset"),
        s.field("fanout_mode", self.fanout_mode, -1, 
            doc="Fanout mode. -1: none, 0: fanout, 1: standalone"),
    ], doc="TimingMasterController configuration"),

};

moo.oschema.sort_select(cs)