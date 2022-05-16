local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.timingfanoutcontrollerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("sent_io_reset_cmds", self.uint8, doc="Number of sent io_reset commands"),
       s.field("sent_print_status_cmds", self.uint8, doc="Number of sent print_status commands"),
       s.field("sent_fanout_endpoint_enable_cmds", self.uint8, doc="Number of sent fanout_endpoint_enable commands"),
       s.field("sent_fanout_endpoint_reset_cmds", self.uint8, doc="Number of sent fanout_endpoint_reset commands"),
   ], doc="TimingFanoutController information")
};

moo.oschema.sort_select(info)