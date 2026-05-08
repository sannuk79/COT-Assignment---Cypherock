#include "proto_codec.h"
#include "cot.pb.h"
#include <pb_encode.h>
#include <pb_decode.h>

#include <algorithm>
#include <cstddef>

std::vector<uint8_t> ProtoCodec::encode_session_config(const SessionConfigMessage& message) {
    SessionConfig pb_msg = SessionConfig_init_default;
    pb_msg.debug_reveal = message.debug_reveal;

    uint8_t buffer[SessionConfig_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, SessionConfig_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> ProtoCodec::encode_ot_init(const OTInitMessage& message) {
    OTInit pb_msg = OTInit_init_default;
    std::copy(message.point_a.begin(), message.point_a.end(), pb_msg.point_a);
    pb_msg.bit_index = message.bit_index;

    uint8_t buffer[OTInit_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, OTInit_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> ProtoCodec::encode_ot_response(const OTResponseMessage& message) {
    OTResponse pb_msg = OTResponse_init_default;
    std::copy(message.point_b.begin(), message.point_b.end(), pb_msg.point_b);

    uint8_t buffer[OTResponse_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, OTResponse_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> ProtoCodec::encode_ot_encrypted(const OTEncryptedPayload& message) {
    OTEncrypted pb_msg = OTEncrypted_init_default;
    std::copy(message.encrypted_m0.begin(), message.encrypted_m0.end(), pb_msg.encrypted_m0);
    std::copy(message.encrypted_m1.begin(), message.encrypted_m1.end(), pb_msg.encrypted_m1);
    std::copy(message.nonce.begin(), message.nonce.end(), pb_msg.nonce);

    uint8_t buffer[OTEncrypted_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, OTEncrypted_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> ProtoCodec::encode_scalar(const ScalarMessage& message) {
    MTAShare pb_msg = MTAShare_init_default;
    std::copy(message.value.begin(), message.value.end(), pb_msg.additive_share);

    uint8_t buffer[MTAShare_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, MTAShare_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

std::vector<uint8_t> ProtoCodec::encode_debug_reveal_response(const DebugRevealResponseMessage& message) {
    DebugRevealResponse pb_msg = DebugRevealResponse_init_default;
    std::copy(message.multiplicative_share_y.begin(), message.multiplicative_share_y.end(), pb_msg.multiplicative_share_y);
    pb_msg.verified = message.verified;

    uint8_t buffer[DebugRevealResponse_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, DebugRevealResponse_fields, &pb_msg)) {
        return {};
    }
    return std::vector<uint8_t>(buffer, buffer + stream.bytes_written);
}

bool ProtoCodec::decode_session_config(const std::vector<uint8_t>& bytes, SessionConfigMessage& out) {
    SessionConfig pb_msg = SessionConfig_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    if (!pb_decode(&stream, SessionConfig_fields, &pb_msg)) {
        return false;
    }
    out.debug_reveal = pb_msg.debug_reveal;
    return true;
}

bool ProtoCodec::decode_ot_init(const std::vector<uint8_t>& bytes, OTInitMessage& out) {
    OTInit pb_msg = OTInit_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    if (!pb_decode(&stream, OTInit_fields, &pb_msg)) {
        return false;
    }
    std::copy(pb_msg.point_a, pb_msg.point_a + 33, out.point_a.begin());
    out.bit_index = pb_msg.bit_index;
    return true;
}

bool ProtoCodec::decode_ot_encrypted(const std::vector<uint8_t>& bytes, OTEncryptedPayload& out) {
    OTEncrypted pb_msg = OTEncrypted_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    if (!pb_decode(&stream, OTEncrypted_fields, &pb_msg)) {
        return false;
    }
    std::copy(pb_msg.encrypted_m0, pb_msg.encrypted_m0 + 32, out.encrypted_m0.begin());
    std::copy(pb_msg.encrypted_m1, pb_msg.encrypted_m1 + 32, out.encrypted_m1.begin());
    std::copy(pb_msg.nonce, pb_msg.nonce + 12, out.nonce.begin());
    return true;
}

bool ProtoCodec::decode_scalar(const std::vector<uint8_t>& bytes, ScalarMessage& out) {
    MTAShare pb_msg = MTAShare_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    if (!pb_decode(&stream, MTAShare_fields, &pb_msg)) {
        return false;
    }
    std::copy(pb_msg.additive_share, pb_msg.additive_share + 32, out.value.begin());
    return true;
}

bool ProtoCodec::decode_debug_reveal_request(const std::vector<uint8_t>& bytes, DebugRevealRequestMessage& out) {
    DebugRevealRequest pb_msg = DebugRevealRequest_init_default;
    pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
    if (!pb_decode(&stream, DebugRevealRequest_fields, &pb_msg)) {
        return false;
    }
    std::copy(pb_msg.multiplicative_share_x, pb_msg.multiplicative_share_x + 32, out.multiplicative_share_x.begin());
    std::copy(pb_msg.additive_share_u, pb_msg.additive_share_u + 32, out.additive_share_u.begin());
    return true;
}
