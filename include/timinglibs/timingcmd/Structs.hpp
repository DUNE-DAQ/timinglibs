/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains struct and other type definitions for shema in 
 * namespace dunedaq::timinglibs::timingcmd.
 */
#ifndef DUNEDAQ_TIMINGLIBS_TIMINGCMD_STRUCTS_HPP
#define DUNEDAQ_TIMINGLIBS_TIMINGCMD_STRUCTS_HPP

#include <cstdint>

#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace dunedaq::timinglibs::timingcmd {

    // @brief A bool
    using BoolData = bool;

    // @brief A double
    using DoubleData = double;


    // @brief An int
    using IntData = int32_t;


    // @brief A PLL register bit(s) value
    using UintData = uint32_t; // NOLINT


    // @brief Endpoint location data
    struct EndpointLocation 
    {

        // @brief Fanout slot of the endpoint
        IntData fanout_slot = 0;

        // @brief SFP slot of the endpoint
        IntData sfp_slot = 0;

        // @brief Address of the endpoint
        UintData address = 0;
    };

    // @brief Structure for payload of hsi configure commands
    struct HSIConfigureCmdPayload 
    {

        // @brief Rising edge mask for HSI triggering
        UintData rising_edge_mask = 0;

        // @brief Falling edge mask for HSI triggering
        UintData falling_edge_mask = 0;

        // @brief Invert edge mask for HSI triggering
        UintData invert_edge_mask = 0;

        // @brief Source of data for HSI triggering
        UintData data_source = 0;

        // @brief Source of data for HSI triggering in emulation (bit 0)
        DoubleData random_rate = 0.0;
    };

    // @brief Name of a target instance of a kind
    using String = std::string;

    // @brief Structure for io reset commands
    struct IOResetCmdPayload 
    {

        // @brief Path of clock config file
        String clock_config = "";

        // @brief Soft reset
        BoolData soft = false;

        // @brief Clock source, 0 for PLL input 0, 1 for in 1, etc.. 255 for free run mode
        UintData clock_source = 0;
    };

    // @brief Structure for io reset commands
    struct SyncTimestampPayload 
    {

        // @brief Timestamp source, 0 for upstream, 1 for software, 2 for mixed
        UintData timestamp_source = 0;
    };

    // @brief Structure for payload of endpoint commands
    struct TimingEndpointCmdPayload 
    {

        // @brief ID of target endpoint
        UintData endpoint_id = 0;
    };

    // @brief Structure for payload of endpoint configure commands
    struct TimingEndpointConfigureCmdPayload 
    {

        // @brief ID of target endpoint
        UintData endpoint_id = 0;

        // @brief Endpoint address
        UintData address = 0;

        // @brief Endpoint partition
        UintData partition = 0;
    };

    // @brief A vector of endpoint locations
    using TimingEndpointLocations = std::vector<dunedaq::timinglibs::timingcmd::EndpointLocation>;

    // @brief The timing hw cmd name.  FIXME: this should be an enum!
    using TimingHwCmdId = std::string;

    // @brief Generic structure for timing hw cmd payloads
    using TimingHwCmdPayload = nlohmann::json;

    // @brief Timing hw cmd structure
    struct TimingHwCmd 
    {

        // @brief ID of hw cmd
        TimingHwCmdId id = "";

        // @brief Cmd target
        String device = "";

        // @brief Hw cmd payload
        TimingHwCmdPayload payload = {};
    };

    // @brief Structure for payloads of endpoint scan configure commands
    struct TimingMasterEndpointScanPayload 
    {

        // @brief List of target endpoint
        TimingEndpointLocations endpoints = {};
    };

    // @brief Structure for payload of endpoint configure commands
    struct TimingMasterSendFLCmdCmdPayload 
    {

        // @brief ID of target endpoint
        UintData fl_cmd_id = 0;

        // @brief Channel on which to send command
        UintData channel = 0;

        // @brief How many commands to send
        UintData number_of_commands_to_send = 0;
    };

    // @brief Structure for payload of timing master set endpoint delay command
    struct TimingMasterSetEndpointDelayCmdPayload 
    {

        // @brief Endpoint address
        UintData address = 0;

        // @brief Endpoint coarse delay
        UintData coarse_delay = 0;

        // @brief Endpoint fine delay
        UintData fine_delay = 0;

        // @brief Endpoint phase delay
        UintData phase_delay = 0;

        // @brief Measure round trip time after delay setting
        BoolData measure_rtt = false;

        // @brief Control SFP or not
        BoolData control_sfp = false;

        // @brief Mux to endpoint (or not)
        IntData sfp_mux = 0;
    };

    // @brief Structure for payload of partition commands
    struct TimingPartitionCmdPayload 
    {

        // @brief ID of target partition
        UintData partition_id = 0;
    };

    // @brief Structure for payload of partition configure commands
    struct TimingPartitionConfigureCmdPayload 
    {

        // @brief ID of target partition
        UintData partition_id = 0;

        // @brief Trigger mask for fixed length cmd distribution
        UintData trigger_mask = 0;

        // @brief Spill interface on
        BoolData spill_gate_enabled = false;

        // @brief Rate control on
        BoolData rate_control_enabled = false;
    };

} // namespace dunedaq::timinglibs::timingcmd

#endif // DUNEDAQ_TIMINGLIBS_TIMINGCMD_STRUCTS_HPP