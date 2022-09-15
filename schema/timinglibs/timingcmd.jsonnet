local moo = import "moo.jsonnet";

// A schema builder in the given path (namespace)
local ns = "dunedaq.timinglibs.timingcmd";
local s = moo.oschema.schema(ns);

// A temporary schema construction context.
local timingcmd = {
    
    bool_data: s.boolean("BoolData", doc="A bool"),

    uint_data: s.number("UintData", "u4", 
        doc="A PLL register bit(s) value"),
    
    int_data: s.number("IntData", "i4", 
        doc="An int"),

    fanout_mode: s.number("FanoutMode", "i4", 
        doc="Fanout mode"),

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),

    inst: s.string("String",
                   doc="Name of a target instance of a kind"),

    timinghwcmdid: s.string("TimingHwCmdId", pattern=moo.re.ident_only,
                    doc="The timing hw cmd name.  FIXME: this should be an enum!"),

    timing_hw_cmd_payload: s.any("TimingHwCmdPayload", 
                    doc="Generic structure for timing hw cmd payloads"),

    timinghwcmd: s.record("TimingHwCmd", [
        s.field("id", self.timinghwcmdid,
                doc="ID of hw cmd"),
        s.field("device", self.inst,
                doc="Cmd target"),
        s.field("payload", self.timing_hw_cmd_payload,
                doc="Hw cmd payload")

    ], doc="Timing hw cmd structure"),

    io_reset_cmd_payload: s.record("IOResetCmdPayload",[
        s.field("clock_config", self.inst, "",
            doc="Path of clock config file"),
        s.field("soft", self.bool_data, false,
            doc="Soft reset"),
        s.field("fanout_mode", self.fanout_mode, -1, 
            doc="Fanout mode. -1: none, 0: fanout, 1: standalone"),
    ], doc="Structure for io reset commands"),

    timing_partition_cmd_payload: s.record("TimingPartitionCmdPayload",[
        s.field("partition_id", self.uint_data,
            doc="ID of target partition"),
    ], doc="Structure for payload of partition commands"),

    timing_partition_configure_cmd_payload: s.record("TimingPartitionConfigureCmdPayload",[
        s.field("partition_id", self.uint_data, optional=true,
            doc="ID of target partition"),
        s.field("trigger_mask", self.uint_data,
            doc="Trigger mask for fixed length cmd distribution"),
        s.field("spill_gate_enabled", self.bool_data, false,
            doc="Spill interface on"),
        s.field("rate_control_enabled", self.bool_data, false,
            doc="Rate control on"),
    ], doc="Structure for payload of partition configure commands"),

    timing_endpoint_cmd_payload: s.record("TimingEndpointCmdPayload",[
        s.field("endpoint_id", self.uint_data,
            doc="ID of target endpoint"),
    ], doc="Structure for payload of endpoint commands"),

    timing_endpoint_configure_cmd_payload: s.record("TimingEndpointConfigureCmdPayload",[
        s.field("endpoint_id", self.uint_data, optional=true,
            doc="ID of target endpoint"),
        s.field("address", self.uint_data,
            doc="Endpoint address"),
        s.field("partition", self.uint_data,
            doc="Endpoint partition"),
    ], doc="Structure for payload of endpoint configure commands"),

    hsi_configure_cmd_payload: s.record("HSIConfigureCmdPayload",[
        s.field("rising_edge_mask", self.uint_data,
            doc="Rising edge mask for HSI triggering"),

        s.field("falling_edge_mask", self.uint_data,
            doc="Falling edge mask for HSI triggering"),
        
        s.field("invert_edge_mask", self.uint_data,
            doc="Invert edge mask for HSI triggering"),
        
        s.field("data_source", self.uint_data,
            doc="Source of data for HSI triggering"),

        s.field("random_rate", self.double_data,
            doc="Source of data for HSI triggering in emulation (bit 0)"),
    ], doc="Structure for payload of hsi configure commands"),

    timing_master_send_fl_cmd_cmd__payload: s.record("TimingMasterSendFLCmdCmdPayload",[
        s.field("fl_cmd_id", self.uint_data,
            doc="ID of target endpoint"),
        s.field("channel", self.uint_data,
            doc="Channel on which to send command"),
        s.field("number_of_commands_to_send", self.uint_data,
            doc="How many commands to send"),
    ], doc="Structure for payload of endpoint configure commands"),

    timing_master_set_endpoint_delay_cmd_payload: s.record("TimingMasterSetEndpointDelayCmdPayload",[
        s.field("address", self.uint_data,
            doc="Endpoint address"),
        s.field("coarse_delay", self.uint_data,
            doc="Endpoint coarse delay"),
        s.field("fine_delay", self.uint_data,
            doc="Endpoint fine delay"),
        s.field("phase_delay", self.uint_data,
            doc="Endpoint phase delay"),
        s.field("measure_rtt", self.bool_data,
            doc="Measure round trip time after delay setting"),
        s.field("control_sfp", self.bool_data,
            doc="Control SFP or not"),
        s.field("sfp_mux", self.int_data,
            doc="Mux to endpoint (or not)"),
    ], doc="Structure for payload of timing master set endpoint delay command"),

    timing_endpoints_addresses: s.sequence("TimingEndpointScanAddresses", self.uint_data,
            doc="A vector of endpoint addresses"),

    timing_master_endpoint_scan_payload: s.record("TimingMasterEndpointScanPayload",[
        s.field("endpoints", self.timing_endpoints_addresses,
            doc="List of target endpoint"),
    ], doc="Structure for payloads of endpoint scan configure commands"),


};

// Output a topologically sorted array.
moo.oschema.sort_select(timingcmd, ns)
