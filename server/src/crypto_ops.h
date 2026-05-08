#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

using Scalar = std::array<uint8_t, 32>;
using Point = std::array<uint8_t, 33>;

class CryptoOps {
public:
  static constexpr const char* SECP256K1_ORDER_HEX =
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";

  static Scalar generate_random_scalar();
  static Scalar mod_add(const Scalar& lhs, const Scalar& rhs);
  static Scalar mod_sub(const Scalar& lhs, const Scalar& rhs);
  static Scalar mod_mul(const Scalar& lhs, const Scalar& rhs);
  static Scalar mod_negate(const Scalar& value);

  static std::optional<Point> scalar_multiply_g(const Scalar& scalar);
  static std::optional<Point> scalar_multiply_point(const Scalar& scalar, const Point& point);
  static std::optional<Point> point_add(const Point& lhs, const Point& rhs);
  static std::optional<Point> point_negate(const Point& point);
  static std::optional<Point> point_subtract(const Point& lhs, const Point& rhs);
  static std::optional<Scalar> get_x_coordinate(const Point& point);

  static Scalar derive_key(const uint8_t* bytes, size_t size);
  static void xor_encrypt(const Scalar& message, const Scalar& key, Scalar& out);

  static std::string hex_encode(const Scalar& scalar);
  static std::string hex_encode(const Point& point);
};
