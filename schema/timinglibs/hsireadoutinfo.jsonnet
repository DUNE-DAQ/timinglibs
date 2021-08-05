local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.timinglibs.hsireadoutinfo");

local info = {
   cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 
    uint4  : s.number("uint4", "u4",
                     doc="An unsigned of 8 bytes"),
    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

    //counter_vector: s.sequence("HwCommandCounters", self.uint8,
    //        doc="A vector hardware command counters"),

    double_val: s.number("DoubleValue", "f8", 
        doc="A double"),
    
   info: s.record("Info", [
       s.field("readout_hsi_events_counter", self.uint8, doc="Number of read HSIEvents so far"), 
       s.field("sent_hsi_events_counter", self.uint8, doc="Number of sent HSIEvents so far"), 
       s.field("failed_to_send_hsi_events_counter", self.uint8, doc="Number of failed send attempts so far"), 
       s.field("last_readout_timestamp", self.uint8, doc="Timestamp of the last read HSIEvent"), 
       s.field("last_sent_timestamp", self.uint8, doc="Timestamp of the last sent HSIEvent"), 
       s.field("average_buffer_occupancy", self.double_val, doc="Average (word) occupancy of buffer in HSI firmware. One HSIEvent is 5 words."), 
   ], doc="HSIReadout information")
};

moo.oschema.sort_select(info)