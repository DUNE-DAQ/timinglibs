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
       s.field("sent_master_set_endpoint_delay_cmds", self.uint8, doc="Number of sent set_endpoint_delay commands"),
       s.field("sent_master_send_fl_command_cmds", self.uint8, doc="Number of sent send_fl_command commands"),
       s.field("sent_master_measure_endpoint_rtt", self.uint8, doc="Number of sent measure_endpoint_rtt commands"),
       s.field("sent_master_endpoint_scan", self.uint8, doc="Number of sent endpoint_scan commands"),
       s.field("sent_master_start_periodic_fl_commands", self.uint8, doc="Number of start_periodic_fl_command commands"),
       s.field("sent_master_stop_periodic_fl_commands", self.uint8, doc="Number of stop_periodic_fl_command commands")

   ], doc="TimingMasterController information")
};

moo.oschema.sort_select(info)