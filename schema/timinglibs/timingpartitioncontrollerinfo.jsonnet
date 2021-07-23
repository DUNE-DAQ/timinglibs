local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.timingpartitioncontrollerinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

   info: s.record("Info", [
      s.field("sent_partition_configure_cmds", self.uint8, doc="Number of sent partition_configure commands"),
      s.field("sent_partition_enable_cmds", self.uint8, doc="Number of sent partition_enable commands"),
      s.field("sent_partition_disable_cmds", self.uint8, doc="Number of sent partition_disable commands"),
      s.field("sent_partition_start_cmds", self.uint8, doc="Number of sent partition_start commands"),
      s.field("sent_partition_stop_cmds", self.uint8, doc="Number of sent partition_stop commands"),
      s.field("sent_partition_enable_triggers_cmds", self.uint8, doc="Number of sent partition_enable_triggers commands"),
      s.field("sent_partition_disable_triggers_cmds", self.uint8, doc="Number of sent partition_disable_triggers commands"),
      s.field("sent_partition_print_status_cmds", self.uint8, doc="Number of sent partition_print_status commands"),
   ], doc="TimingPartitionController information")
};

moo.oschema.sort_select(info)