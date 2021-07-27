local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.hsicontrollerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

    counter_vector: s.sequence("HwCommandCounters", self.uint8,
            doc="A vector hardware command counters"),

   info: s.record("Info", [
      //s.field("sent_hw_command_counters", self.counter_vector, doc="Number of hw commands sent so far"),
      s.field("sent_hsi_io_reset_cmds", self.uint8, doc="Number of sent hsi_io_reset commands"),
      s.field("sent_hsi_endpoint_enable_cmds", self.uint8, doc="Number of sent hsi_endpoint_enable commands"),
      s.field("sent_hsi_endpoint_disable_cmds", self.uint8, doc="Number of sent hsi_endpoint_disable commands"),
      s.field("sent_hsi_endpoint_reset_cmds", self.uint8, doc="Number of sent hsi_endpoint_reset commands"),
      s.field("sent_hsi_reset_cmds", self.uint8, doc="Number of sent hsi_reset commands"),
      s.field("sent_hsi_configure_cmds", self.uint8, doc="Number of sent hsi_configure commands"),
      s.field("sent_hsi_start_cmds", self.uint8, doc="Number of sent hsi_start commands"),
      s.field("sent_hsi_stop_cmds", self.uint8, doc="Number of sent hsi_stop commands"),
      s.field("sent_hsi_print_status_cmds", self.uint8, doc="Number of sent hsi_print_status commands"),
   ], doc="HSIController information")
};

moo.oschema.sort_select(info)