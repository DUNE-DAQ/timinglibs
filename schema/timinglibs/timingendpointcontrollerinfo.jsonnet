local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.timingendpointcontrollerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
      s.field("sent_endpoint_io_reset_cmds", self.uint8, doc="Number of sent endpoint_io_reset commands"),
      s.field("sent_endpoint_enable_cmds", self.uint8, doc="Number of sent endpoint_enable commands"),
      s.field("sent_endpoint_disable_cmds", self.uint8, doc="Number of sent endpoint_disable commands"),
      s.field("sent_endpoint_reset_cmds", self.uint8, doc="Number of sent endpoint_reset commands"),
      s.field("sent_endpoint_print_status_cmds", self.uint8, doc="Number of sent endpoint_print_status commands"),
      s.field("sent_endpoint_print_timestamp_cmds", self.uint8, doc="Number of sent endpoint_print_timestamp commands"),
   ], doc="TimingEndpointController information")
};

moo.oschema.sort_select(info)