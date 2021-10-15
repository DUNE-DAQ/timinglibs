local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.hsicontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {
	
    str : s.string("Str", doc="A string field"),
    
    size: s.number("Size", "u8",
        doc="A count of very many things"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),

    init: s.record("InitParams", [
        s.field("qinfos", app.QueueInfos,
                doc="Information for a module to find its queue"),
        s.field("device", self.str, "",
                doc="String of managed device name"),
    ], doc="HSIController configuration"),

    conf: s.record("ConfParams",[
        s.field("address", self.uint_data,
            doc="HSI endpoint address"),
        s.field("partition", self.uint_data,
            doc="HSI endpoint partition"),
        s.field("rising_edge_mask", self.uint_data,
            doc="Rising edge mask for HSI triggering"),
        s.field("falling_edge_mask", self.uint_data,
            doc="Falling edge mask for HSI triggering"),
        s.field("invert_edge_mask", self.uint_data,
            doc="Invert edge mask for HSI triggering"),
        s.field("data_source", self.uint_data,
            doc="Source of data for HSI triggering"),
        s.field("random_rate", self.double_data,
            doc="Source of data for HSI triggering in emulation (bit 0)"),
    ], doc="Structure for payload of hsi configure commands"),
};

s_app + moo.oschema.sort_select(cs)