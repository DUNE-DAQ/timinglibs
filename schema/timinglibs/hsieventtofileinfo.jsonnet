local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.hsieventtofileinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint4  : s.number("uint4", "u4",
                     doc="An unsigned of 8 bytes"),
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

    double_val: s.number("DoubleValue", "f8", 
        doc="A double"),
    
   info: s.record("Info", [
      s.field("received_counter", self.uint8, doc="Number of received HSIEvents so far"), 
      s.field("last_received_timestamp", self.uint8, doc="Timestamp of the last received HSIEvent"), 

      s.field("written_counter", self.uint8, doc="Number of written HSIEvents so far"), 
      s.field("failed_to_write_counter", self.uint8, doc="Number of failed write attempts so far"),
      s.field("last_written_timestamp", self.uint8, doc="Timestamp of the last written HSIEvent"), 

   ], doc="HSIEventToFile information")
};

moo.oschema.sort_select(info)