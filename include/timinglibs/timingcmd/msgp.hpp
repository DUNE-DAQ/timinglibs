/*
 * This file is 100% generated.  Any manual edits will likely be lost.
 *
 * This contains functions struct and other type definitions for schema in
 * namespace dunedaq::timinglibs::timingcmd to be serialized via MsgPack.
 */
#ifndef DUNEDAQ_TIMINGLIBS_TIMINGCMD_MSGPACK_HPP
#define DUNEDAQ_TIMINGLIBS_TIMINGCMD_MSGPACK_HPP

// My structs
#include "timinglibs/timingcmd/Structs.hpp"


// We have ANY types so need to include NLJS serialization
#include "timinglibs/timingcmd/Nljs.hpp"

#include <msgpack.hpp>


// MsgPack serialization/deserialization functions
namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

// MsgPack serialization for ANY type:
// dunedaq::timinglibs::timingcmd::TimingHwCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingHwCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingHwCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 1) throw msgpack::type_error();
        v=dunedaq::timinglibs::timingcmd::TimingHwCmdPayload::parse(o.via.array.ptr[0].as<std::string>());
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingHwCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingHwCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(1);
        o.pack(v.dump());
        return o;
    }
};


// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::EndpointLocation
template<>
struct convert<dunedaq::timinglibs::timingcmd::EndpointLocation> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::EndpointLocation& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v.fanout_slot = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::IntData>();
        v.sfp_slot = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::IntData>();
        v.address = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::EndpointLocation> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::EndpointLocation const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.fanout_slot);
        o.pack(v.sfp_slot);
        o.pack(v.address);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::HSIConfigureCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::HSIConfigureCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::HSIConfigureCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 5) throw msgpack::type_error();
        v.rising_edge_mask = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.falling_edge_mask = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.invert_edge_mask = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.data_source = o.via.array.ptr[3].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.random_rate = o.via.array.ptr[4].as<dunedaq::timinglibs::timingcmd::DoubleData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::HSIConfigureCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::HSIConfigureCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(5);
        o.pack(v.rising_edge_mask);
        o.pack(v.falling_edge_mask);
        o.pack(v.invert_edge_mask);
        o.pack(v.data_source);
        o.pack(v.random_rate);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::IOResetCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::IOResetCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::IOResetCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v.clock_config = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::String>();
        v.soft = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::BoolData>();
        v.clock_source = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::IOResetCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::IOResetCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.clock_config);
        o.pack(v.soft);
        o.pack(v.clock_source);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::SyncTimestampPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::SyncTimestampPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::SyncTimestampPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 1) throw msgpack::type_error();
        v.timestamp_source = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::SyncTimestampPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::SyncTimestampPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(1);
        o.pack(v.timestamp_source);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingEndpointCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingEndpointCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingEndpointCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 1) throw msgpack::type_error();
        v.endpoint_id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingEndpointCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingEndpointCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(1);
        o.pack(v.endpoint_id);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingEndpointConfigureCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingEndpointConfigureCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingEndpointConfigureCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v.endpoint_id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.address = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.partition = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingEndpointConfigureCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingEndpointConfigureCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.endpoint_id);
        o.pack(v.address);
        o.pack(v.partition);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingHwCmd
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingHwCmd> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingHwCmd& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v.id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::TimingHwCmdId>();
        v.device = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::String>();
        v.payload = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::TimingHwCmdPayload>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingHwCmd> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingHwCmd const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.id);
        o.pack(v.device);
        o.pack(v.payload);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingMasterEndpointScanPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingMasterEndpointScanPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingMasterEndpointScanPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 1) throw msgpack::type_error();
        v.endpoints = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::TimingEndpointLocations>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingMasterEndpointScanPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingMasterEndpointScanPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(1);
        o.pack(v.endpoints);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingMasterSendFLCmdCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingMasterSendFLCmdCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingMasterSendFLCmdCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 3) throw msgpack::type_error();
        v.fl_cmd_id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.channel = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.number_of_commands_to_send = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingMasterSendFLCmdCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingMasterSendFLCmdCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(3);
        o.pack(v.fl_cmd_id);
        o.pack(v.channel);
        o.pack(v.number_of_commands_to_send);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingMasterSetEndpointDelayCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingMasterSetEndpointDelayCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingMasterSetEndpointDelayCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 7) throw msgpack::type_error();
        v.address = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.coarse_delay = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.fine_delay = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.phase_delay = o.via.array.ptr[3].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.measure_rtt = o.via.array.ptr[4].as<dunedaq::timinglibs::timingcmd::BoolData>();
        v.control_sfp = o.via.array.ptr[5].as<dunedaq::timinglibs::timingcmd::BoolData>();
        v.sfp_mux = o.via.array.ptr[6].as<dunedaq::timinglibs::timingcmd::IntData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingMasterSetEndpointDelayCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingMasterSetEndpointDelayCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(7);
        o.pack(v.address);
        o.pack(v.coarse_delay);
        o.pack(v.fine_delay);
        o.pack(v.phase_delay);
        o.pack(v.measure_rtt);
        o.pack(v.control_sfp);
        o.pack(v.sfp_mux);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingPartitionCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingPartitionCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingPartitionCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 1) throw msgpack::type_error();
        v.partition_id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingPartitionCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingPartitionCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(1);
        o.pack(v.partition_id);
        return o;
    }
};

// MsgPack serialization for RECORD type:
// dunedaq::timinglibs::timingcmd::TimingPartitionConfigureCmdPayload
template<>
struct convert<dunedaq::timinglibs::timingcmd::TimingPartitionConfigureCmdPayload> {
    msgpack::object const& operator()(msgpack::object const& o, dunedaq::timinglibs::timingcmd::TimingPartitionConfigureCmdPayload& v) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 4) throw msgpack::type_error();
        v.partition_id = o.via.array.ptr[0].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.trigger_mask = o.via.array.ptr[1].as<dunedaq::timinglibs::timingcmd::UintData>();
        v.spill_gate_enabled = o.via.array.ptr[2].as<dunedaq::timinglibs::timingcmd::BoolData>();
        v.rate_control_enabled = o.via.array.ptr[3].as<dunedaq::timinglibs::timingcmd::BoolData>();
        return o;
    }
};
template<>
struct pack<dunedaq::timinglibs::timingcmd::TimingPartitionConfigureCmdPayload> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, dunedaq::timinglibs::timingcmd::TimingPartitionConfigureCmdPayload const& v) const {
        // packing member variables as an array.
        o.pack_array(4);
        o.pack(v.partition_id);
        o.pack(v.trigger_mask);
        o.pack(v.spill_gate_enabled);
        o.pack(v.rate_control_enabled);
        return o;
    }
};


} // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

#endif // DUNEDAQ_TIMINGLIBS_TIMINGCMD_MSGPACK_HPP