#include "ot_protocol.h"
extern "C" {
#include "bignum.h"
#include "secp256k1.h"
}

#include <algorithm>
#include <cstring>

namespace {
const bignum256& order() {
    return secp256k1.order;
}

void scalar_to_bn(const Scalar& scalar, bignum256* out) {
    bn_read_be(scalar.data(), out);
}

void bn_to_scalar(const bignum256* in, Scalar& out) {
    bn_write_be(in, out.data());
}
} // namespace

BobOT::BobOT(uint8_t choice_bit)
    : choice_bit_(choice_bit), secret_b_(CryptoOps::generate_random_scalar()) {}

std::optional<Point> BobOT::process_init(const Point& a_point) {
    auto shared = CryptoOps::scalar_multiply_point(secret_b_, a_point);
    if (!shared) return std::nullopt;

    auto x = CryptoOps::get_x_coordinate(*shared);
    if (!x) return std::nullopt;

    decryption_key_ = CryptoOps::derive_key(x->data(), x->size());

    auto b_g = CryptoOps::scalar_multiply_g(secret_b_);
    if (!b_g) return std::nullopt;

    if (choice_bit_ == 0) {
        return *b_g;
    }

    return CryptoOps::point_add(*b_g, a_point);
}

bool BobOT::decrypt_message(const OTEncryptedPayload& encrypted, Scalar& out) const {
    if (!decryption_key_) return false;

    const Scalar& selected = (choice_bit_ == 0) ? encrypted.encrypted_m0 : encrypted.encrypted_m1;
    CryptoOps::xor_encrypt(selected, *decryption_key_, out);
    return true;
}

Scalar MTA::compute_alice_share(const std::array<Scalar, 256>& ui_values) {
    bignum256 sum, n, term, two_pow_i, ui_bn;
    bn_zero(&sum);
    n = order();
    
    // two_pow_i = 2^0 % n = 1
    bn_zero(&two_pow_i);
    two_pow_i.val[0] = 1;

    for (size_t i = 0; i < 256; ++i) {
        scalar_to_bn(ui_values[i], &ui_bn);
        
        // term = (ui[i] * 2^i) % n
        bn_copy(&term, &ui_bn);
        bn_multiply(&two_pow_i, &term, &n);
        
        // sum = (sum + term) % n
        bn_addmod(&sum, &term, &n);
        
        // two_pow_i = (two_pow_i * 2) % n
        bn_addmod(&two_pow_i, &two_pow_i, &n);
    }

    if (bn_is_zero(&sum)) {
        return Scalar{};
    }

    // Alice's share u = -sum % n = (n - sum) % n
    bignum256 u_bn;
    bignum256 zero;
    bn_zero(&zero);
    bn_subtractmod(&zero, &sum, &u_bn, &n);
    bn_mod(&u_bn, &n);
    bn_mod(&u_bn, &n);
    
    Scalar out;
    bn_to_scalar(&u_bn, out);
    return out;
}

Scalar MTA::compute_bob_share(const Scalar& y, const std::array<Scalar, 256>& mc_values) {
    (void)y;
    bignum256 sum, n, term, two_pow_i, mc_bn;
    bn_zero(&sum);
    n = order();
    
    bn_zero(&two_pow_i);
    two_pow_i.val[0] = 1;

    for (size_t i = 0; i < 256; ++i) {
        scalar_to_bn(mc_values[i], &mc_bn);
        
        bn_copy(&term, &mc_bn);
        bn_multiply(&two_pow_i, &term, &n);
        
        bn_addmod(&sum, &term, &n);
        
        bn_addmod(&two_pow_i, &two_pow_i, &n);
    }

    bn_mod(&sum, &n);
    Scalar out;
    bn_to_scalar(&sum, out);
    return out;
}

bool MTA::verify_mta(const Scalar& x, const Scalar& y, const Scalar& u, const Scalar& v) {
    bignum256 bn_x, bn_y, bn_u, bn_v, bn_xy, bn_uv, n;
    n = order();
    
    scalar_to_bn(x, &bn_x);
    scalar_to_bn(y, &bn_y);
    scalar_to_bn(u, &bn_u);
    scalar_to_bn(v, &bn_v);
    
    bn_copy(&bn_xy, &bn_x);
    bn_multiply(&bn_y, &bn_xy, &n);
    
    bn_copy(&bn_uv, &bn_u);
    bn_addmod(&bn_uv, &bn_v, &n);
    
    return bn_is_equal(&bn_xy, &bn_uv);
}
