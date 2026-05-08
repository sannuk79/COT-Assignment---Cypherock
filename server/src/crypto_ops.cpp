#include "crypto_ops.h"

extern "C" {
#include "bignum.h"
#include "ecdsa.h"
#include "secp256k1.h"
#include "sha2.h"
}

#include <algorithm>
#include <iomanip>
#include <optional>
#include <random>
#include <sstream>
#include <string>

namespace {
const ecdsa_curve& curve() {
  return secp256k1;
}

const bignum256& order() {
  return curve().order;
}

bool is_valid_scalar(const Scalar& scalar) {
  bignum256 value{};
  bn_read_be(scalar.data(), &value);
  return !bn_is_zero(&value) && bn_is_less(&value, &order());
}

bignum256 scalar_to_bn(const Scalar& scalar) {
  bignum256 value{};
  bn_read_be(scalar.data(), &value);
  return value;
}

Scalar bn_to_scalar(const bignum256& value) {
  Scalar out{};
  bn_write_be(&value, out.data());
  return out;
}

std::optional<curve_point> decode_point(const Point& point) {
  curve_point decoded{};
  if (!ecdsa_read_pubkey(&curve(), point.data(), &decoded)) {
    return std::nullopt;
  }
  return decoded;
}

Point encode_point(const curve_point& point) {
  Point out{};
  compress_coords(&point, out.data());
  return out;
}

std::string hex_encode_bytes(const uint8_t* data, size_t size) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  for (size_t i = 0; i < size; ++i) {
    oss << std::setw(2) << static_cast<int>(data[i]);
  }
  return oss.str();
}
}  // namespace

Scalar CryptoOps::generate_random_scalar() {
  static std::random_device rd;

  while (true) {
    Scalar out{};
    for (auto& byte : out) {
      byte = static_cast<uint8_t>(rd());
    }
    if (is_valid_scalar(out)) {
      return out;
    }
  }
}

Scalar CryptoOps::mod_add(const Scalar& lhs, const Scalar& rhs) {
  bignum256 sum = scalar_to_bn(lhs);
  const bignum256 other = scalar_to_bn(rhs);
  bn_addmod(&sum, &other, &order());
  bn_mod(&sum, &order());
  return bn_to_scalar(sum);
}

Scalar CryptoOps::mod_sub(const Scalar& lhs, const Scalar& rhs) {
  bignum256 result{};
  const bignum256 left = scalar_to_bn(lhs);
  const bignum256 right = scalar_to_bn(rhs);
  bn_subtractmod(&left, &right, &result, &order());
  bn_mod(&result, &order());
  bn_mod(&result, &order());
  return bn_to_scalar(result);
}

Scalar CryptoOps::mod_mul(const Scalar& lhs, const Scalar& rhs) {
  bignum256 result = scalar_to_bn(lhs);
  const bignum256 factor = scalar_to_bn(rhs);
  bn_multiply(&factor, &result, &order());
  bn_mod(&result, &order());
  return bn_to_scalar(result);
}

Scalar CryptoOps::mod_negate(const Scalar& value) {
  if (std::all_of(value.begin(), value.end(), [](uint8_t byte) { return byte == 0; })) {
    return value;
  }

  bignum256 result{};
  const bignum256 zero = {};
  const bignum256 input = scalar_to_bn(value);
  bn_subtractmod(&zero, &input, &result, &order());
  bn_mod(&result, &order());
  bn_mod(&result, &order());
  return bn_to_scalar(result);
}

std::optional<Point> CryptoOps::scalar_multiply_g(const Scalar& scalar) {
  if (!is_valid_scalar(scalar)) {
    return std::nullopt;
  }

  Point out{};
  ecdsa_get_public_key33(&curve(), scalar.data(), out.data());
  return out;
}

std::optional<Point> CryptoOps::scalar_multiply_point(const Scalar& scalar, const Point& point) {
  if (!is_valid_scalar(scalar)) {
    return std::nullopt;
  }

  auto decoded = decode_point(point);
  if (!decoded) {
    return std::nullopt;
  }

  const bignum256 factor = scalar_to_bn(scalar);
  curve_point result{};
  point_multiply(&curve(), &factor, &*decoded, &result);
  return encode_point(result);
}

std::optional<Point> CryptoOps::point_add(const Point& lhs, const Point& rhs) {
  auto left = decode_point(lhs);
  auto right = decode_point(rhs);
  if (!left || !right) {
    return std::nullopt;
  }

  ::point_add(&curve(), &*left, &*right);
  if (point_is_infinity(&*right)) {
    return std::nullopt;
  }

  return encode_point(*right);
}

std::optional<Point> CryptoOps::point_negate(const Point& point) {
  const uint8_t prefix = point[0];
  if (prefix != 0x02 && prefix != 0x03) {
    return std::nullopt;
  }

  Point negated = point;
  negated[0] = prefix == 0x02 ? 0x03 : 0x02;
  return negated;
}

std::optional<Point> CryptoOps::point_subtract(const Point& lhs, const Point& rhs) {
  auto left = decode_point(lhs);
  auto right = decode_point(rhs);
  if (!left || !right) {
    return std::nullopt;
  }

  bignum256 neg_y{};
  bn_subtractmod(&curve().prime, &right->y, &neg_y, &curve().prime);
  right->y = neg_y;

  ::point_add(&curve(), &*right, &*left);
  if (point_is_infinity(&*left)) {
    return std::nullopt;
  }

  return encode_point(*left);
}

std::optional<Scalar> CryptoOps::get_x_coordinate(const Point& point) {
  if (point[0] != 0x02 && point[0] != 0x03) {
    return std::nullopt;
  }

  Scalar x{};
  std::copy(point.begin() + 1, point.end(), x.begin());
  return x;
}

Scalar CryptoOps::derive_key(const uint8_t* bytes, size_t size) {
  Scalar out{};
  sha256_Raw(bytes, size, out.data());
  return out;
}

void CryptoOps::xor_encrypt(const Scalar& message, const Scalar& key, Scalar& out) {
  for (size_t i = 0; i < out.size(); ++i) {
    out[i] = message[i] ^ key[i];
  }
}

std::string CryptoOps::hex_encode(const Scalar& scalar) {
  return hex_encode_bytes(scalar.data(), scalar.size());
}

std::string CryptoOps::hex_encode(const Point& point) {
  return hex_encode_bytes(point.data(), point.size());
}
