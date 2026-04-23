/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains functions struct and other type definitions for shema in 
 * namespace dunedaq::timinglibs::timingcmd to be serialized via nlohmann::json.
 */
#ifndef DUNEDAQ_TIMINGLIBS_TIMINGCMD_NLJS_HPP
#define DUNEDAQ_TIMINGLIBS_TIMINGCMD_NLJS_HPP

// My structs
#include "timinglibs/timingcmd/Structs.hpp"


#include <nlohmann/json.hpp>

namespace dunedaq::timinglibs::timingcmd {

    using data_t = nlohmann::json;
    
    inline void to_json(data_t& j, const EndpointLocation& obj) {
        j["fanout_slot"] = obj.fanout_slot;
        j["sfp_slot"] = obj.sfp_slot;
        j["address"] = obj.address;
    }
    
    inline void from_json(const data_t& j, EndpointLocation& obj) {
        if (j.contains("fanout_slot"))
            j.at("fanout_slot").get_to(obj.fanout_slot);    
        if (j.contains("sfp_slot"))
            j.at("sfp_slot").get_to(obj.sfp_slot);    
        if (j.contains("address"))
            j.at("address").get_to(obj.address);    
    }
    
    inline void to_json(data_t& j, const HSIConfigureCmdPayload& obj) {
        j["rising_edge_mask"] = obj.rising_edge_mask;
        j["falling_edge_mask"] = obj.falling_edge_mask;
        j["invert_edge_mask"] = obj.invert_edge_mask;
        j["data_source"] = obj.data_source;
        j["random_rate"] = obj.random_rate;
    }
    
    inline void from_json(const data_t& j, HSIConfigureCmdPayload& obj) {
        if (j.contains("rising_edge_mask"))
            j.at("rising_edge_mask").get_to(obj.rising_edge_mask);    
        if (j.contains("falling_edge_mask"))
            j.at("falling_edge_mask").get_to(obj.falling_edge_mask);    
        if (j.contains("invert_edge_mask"))
            j.at("invert_edge_mask").get_to(obj.invert_edge_mask);    
        if (j.contains("data_source"))
            j.at("data_source").get_to(obj.data_source);    
        if (j.contains("random_rate"))
            j.at("random_rate").get_to(obj.random_rate);    
    }
    
    inline void to_json(data_t& j, const IOResetCmdPayload& obj) {
        j["clock_config"] = obj.clock_config;
        j["soft"] = obj.soft;
        j["clock_source"] = obj.clock_source;
    }
    
    inline void from_json(const data_t& j, IOResetCmdPayload& obj) {
        if (j.contains("clock_config"))
            j.at("clock_config").get_to(obj.clock_config);    
        if (j.contains("soft"))
            j.at("soft").get_to(obj.soft);    
        if (j.contains("clock_source"))
            j.at("clock_source").get_to(obj.clock_source);    
    }
    
    inline void to_json(data_t& j, const SyncTimestampPayload& obj) {
        j["timestamp_source"] = obj.timestamp_source;
    }
    
    inline void from_json(const data_t& j, SyncTimestampPayload& obj) {
        if (j.contains("timestamp_source"))
            j.at("timestamp_source").get_to(obj.timestamp_source);    
    }
    
    inline void to_json(data_t& j, const TimingEndpointCmdPayload& obj) {
        j["endpoint_id"] = obj.endpoint_id;
    }
    
    inline void from_json(const data_t& j, TimingEndpointCmdPayload& obj) {
        if (j.contains("endpoint_id"))
            j.at("endpoint_id").get_to(obj.endpoint_id);    
    }
    
    inline void to_json(data_t& j, const TimingEndpointConfigureCmdPayload& obj) {
        j["endpoint_id"] = obj.endpoint_id;
        j["address"] = obj.address;
        j["partition"] = obj.partition;
    }
    
    inline void from_json(const data_t& j, TimingEndpointConfigureCmdPayload& obj) {
        if (j.contains("endpoint_id"))
            j.at("endpoint_id").get_to(obj.endpoint_id);    
        if (j.contains("address"))
            j.at("address").get_to(obj.address);    
        if (j.contains("partition"))
            j.at("partition").get_to(obj.partition);    
    }
    
    inline void to_json(data_t& j, const TimingHwCmd& obj) {
        j["id"] = obj.id;
        j["device"] = obj.device;
        j["payload"] = obj.payload;
    }
    
    inline void from_json(const data_t& j, TimingHwCmd& obj) {
        if (j.contains("id"))
            j.at("id").get_to(obj.id);    
        if (j.contains("device"))
            j.at("device").get_to(obj.device);    
        obj.payload = j.at("payload");
    }
    
    inline void to_json(data_t& j, const TimingMasterEndpointScanPayload& obj) {
        j["endpoints"] = obj.endpoints;
    }
    
    inline void from_json(const data_t& j, TimingMasterEndpointScanPayload& obj) {
        if (j.contains("endpoints"))
            j.at("endpoints").get_to(obj.endpoints);    
    }
    
    inline void to_json(data_t& j, const TimingMasterSendFLCmdCmdPayload& obj) {
        j["fl_cmd_id"] = obj.fl_cmd_id;
        j["channel"] = obj.channel;
        j["number_of_commands_to_send"] = obj.number_of_commands_to_send;
    }
    
    inline void from_json(const data_t& j, TimingMasterSendFLCmdCmdPayload& obj) {
        if (j.contains("fl_cmd_id"))
            j.at("fl_cmd_id").get_to(obj.fl_cmd_id);    
        if (j.contains("channel"))
            j.at("channel").get_to(obj.channel);    
        if (j.contains("number_of_commands_to_send"))
            j.at("number_of_commands_to_send").get_to(obj.number_of_commands_to_send);    
    }
    
    inline void to_json(data_t& j, const TimingMasterSetEndpointDelayCmdPayload& obj) {
        j["address"] = obj.address;
        j["coarse_delay"] = obj.coarse_delay;
        j["fine_delay"] = obj.fine_delay;
        j["phase_delay"] = obj.phase_delay;
        j["measure_rtt"] = obj.measure_rtt;
        j["control_sfp"] = obj.control_sfp;
        j["sfp_mux"] = obj.sfp_mux;
    }
    
    inline void from_json(const data_t& j, TimingMasterSetEndpointDelayCmdPayload& obj) {
        if (j.contains("address"))
            j.at("address").get_to(obj.address);    
        if (j.contains("coarse_delay"))
            j.at("coarse_delay").get_to(obj.coarse_delay);    
        if (j.contains("fine_delay"))
            j.at("fine_delay").get_to(obj.fine_delay);    
        if (j.contains("phase_delay"))
            j.at("phase_delay").get_to(obj.phase_delay);    
        if (j.contains("measure_rtt"))
            j.at("measure_rtt").get_to(obj.measure_rtt);    
        if (j.contains("control_sfp"))
            j.at("control_sfp").get_to(obj.control_sfp);    
        if (j.contains("sfp_mux"))
            j.at("sfp_mux").get_to(obj.sfp_mux);    
    }
    
    inline void to_json(data_t& j, const TimingPartitionCmdPayload& obj) {
        j["partition_id"] = obj.partition_id;
    }
    
    inline void from_json(const data_t& j, TimingPartitionCmdPayload& obj) {
        if (j.contains("partition_id"))
            j.at("partition_id").get_to(obj.partition_id);    
    }
    
    inline void to_json(data_t& j, const TimingPartitionConfigureCmdPayload& obj) {
        j["partition_id"] = obj.partition_id;
        j["trigger_mask"] = obj.trigger_mask;
        j["spill_gate_enabled"] = obj.spill_gate_enabled;
        j["rate_control_enabled"] = obj.rate_control_enabled;
    }
    
    inline void from_json(const data_t& j, TimingPartitionConfigureCmdPayload& obj) {
        if (j.contains("partition_id"))
            j.at("partition_id").get_to(obj.partition_id);    
        if (j.contains("trigger_mask"))
            j.at("trigger_mask").get_to(obj.trigger_mask);    
        if (j.contains("spill_gate_enabled"))
            j.at("spill_gate_enabled").get_to(obj.spill_gate_enabled);    
        if (j.contains("rate_control_enabled"))
            j.at("rate_control_enabled").get_to(obj.rate_control_enabled);    
    }
    
} // namespace dunedaq::timinglibs::timingcmd

#endif // DUNEDAQ_TIMINGLIBS_TIMINGCMD_NLJS_HPP