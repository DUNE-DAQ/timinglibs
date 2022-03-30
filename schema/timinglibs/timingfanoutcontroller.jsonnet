local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.timingfanoutcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    str : s.string("Str", doc="A string field"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    init: s.record("ConfParams", [
        s.field("device", self.str, "",
                doc="String of managed device name"),
    ], doc="TimingFanoutController configuration"),

};

s_app + moo.oschema.sort_select(cs)