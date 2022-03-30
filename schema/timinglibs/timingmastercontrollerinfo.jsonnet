local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.timingmastercontrollerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
       s.field("sent_master_io_reset_cmds", self.uint8, doc="Number of sent io_reset commands"),
       s.field("sent_master_set_timestamp_cmds", self.uint8, doc="Number of sent set_timestamp commands"),
       s.field("sent_master_print_status_cmds", self.uint8, doc="Number of sent print_status commands"),
       s.field("sent_set_endpoint_delay_cmds", self.uint8, doc="Number of sent set_endpoint_delay commands"),
   ], doc="TimingMasterController information")
};

moo.oschema.sort_select(info)