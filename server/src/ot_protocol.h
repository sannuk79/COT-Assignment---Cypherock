#pragma once

#include "crypto_ops.h"

#include <array>
#include <cstdint>
#include <optional>

struct OTEncryptedPayload {
  Scalar encrypted_m0{};
  Scalar encrypted_m1{};
  std::array<uint8_t, 12> nonce{};
};

class BobOT {
public:
  explicit BobOT(uint8_t choice_bit);

  std::optional<Point> process_init(const Point& a_point);
  bool decrypt_message(const OTEncryptedPayload& encrypted, Scalar& out) const;

private:
  uint8_t choice_bit_;
  Scalar secret_b_;
  std::optional<Scalar> decryption_key_;
};

class MTA {
public:
  static Scalar compute_alice_share(const std::array<Scalar, 256>& ui_values);
  static Scalar compute_bob_share(const Scalar& y, const std::array<Scalar, 256>& mc_values);
  static bool verify_mta(const Scalar& x, const Scalar& y, const Scalar& u, const Scalar& v);
};
