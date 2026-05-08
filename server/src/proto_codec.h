#pragma once

#include "ot_protocol.h"

#include <cstdint>
#include <vector>

struct OTInitMessage {
  Point point_a{};
  uint32_t bit_index = 0;
};

struct OTResponseMessage {
  Point point_b{};
};

struct ScalarMessage {
  Scalar value{};
};

struct SessionConfigMessage {
  bool debug_reveal = false;
};

struct DebugRevealRequestMessage {
  Scalar multiplicative_share_x{};
  Scalar additive_share_u{};
};

struct DebugRevealResponseMessage {
  Scalar multiplicative_share_y{};
  bool verified = false;
};

class ProtoCodec {
public:
  static std::vector<uint8_t> encode_session_config(const SessionConfigMessage& message);
  static std::vector<uint8_t> encode_ot_init(const OTInitMessage& message);
  static std::vector<uint8_t> encode_ot_response(const OTResponseMessage& message);
  static std::vector<uint8_t> encode_ot_encrypted(const OTEncryptedPayload& message);
  static std::vector<uint8_t> encode_scalar(const ScalarMessage& message);
  static std::vector<uint8_t> encode_debug_reveal_response(const DebugRevealResponseMessage& message);

  static bool decode_session_config(const std::vector<uint8_t>& bytes, SessionConfigMessage& out);
  static bool decode_ot_init(const std::vector<uint8_t>& bytes, OTInitMessage& out);
  static bool decode_ot_encrypted(const std::vector<uint8_t>& bytes, OTEncryptedPayload& out);
  static bool decode_scalar(const std::vector<uint8_t>& bytes, ScalarMessage& out);
  static bool decode_debug_reveal_request(const std::vector<uint8_t>& bytes, DebugRevealRequestMessage& out);
};
