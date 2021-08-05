local moo = import "moo.jsonnet";
local ns = "dunedaq.timinglibs.hsieventtofile";
local s = moo.oschema.schema(ns);

local types = {
    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    count : s.number("Count", "i4",
        doc="A count of not too many things"),

    str : s.string("Str", doc="A string field"),

    conf: s.record("ConfParams", [
        s.field("file_path", self.str, "./",
                doc="file path"),
        s.field("file_prefix", self.str, "hsievents_out_",
                doc="Hardware device poll period [us]"),
    ], doc="HSIEventToFile configuration"),

};

moo.oschema.sort_select(types, ns)
