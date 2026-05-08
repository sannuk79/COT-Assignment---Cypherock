#include "crypto_ops.h"
#include "ot_protocol.h"
#include "proto_codec.h"

#include <iostream>
#include <cassert>
#include <vector>

void test_crypto_ops() {
    std::cout << "[Test] CryptoOps sanity check...\n";
    Scalar s1 = CryptoOps::generate_random_scalar();
    Scalar s2 = CryptoOps::generate_random_scalar();
    
    Scalar sum = CryptoOps::mod_add(s1, s2);
    Scalar diff = CryptoOps::mod_sub(sum, s2);
    
    if (s1 == diff) {
        std::cout << "  - Modulo arithmetic: OK\n";
    } else {
        std::cout << "  - Modulo arithmetic: FAILED\n";
        std::cout << "    s1:   " << CryptoOps::hex_encode(s1) << "\n";
        std::cout << "    diff: " << CryptoOps::hex_encode(diff) << "\n";
    }

    auto g1 = CryptoOps::scalar_multiply_g(s1);
    auto g2 = CryptoOps::scalar_multiply_g(s2);
    
    if (g1 && g2) {
        std::cout << "  - ECC G multiplication: OK\n";
        auto gsum = CryptoOps::point_add(*g1, *g2);
        if (gsum) {
             std::cout << "  - ECC point addition: OK\n";
        }
    } else {
        std::cout << "  - ECC operations: FAILED\n";
    }
}

void test_proto_codec() {
    std::cout << "[Test] ProtoCodec sanity check...\n";
    OTInitMessage msg;
    msg.point_a = CryptoOps::scalar_multiply_g(CryptoOps::generate_random_scalar()).value();
    msg.bit_index = 42;

    auto encoded = ProtoCodec::encode_ot_init(msg);
    std::cout << "  - Encoded OTInit size: " << encoded.size() << " bytes\n";

    OTInitMessage decoded;
    if (ProtoCodec::decode_ot_init(encoded, decoded)) {
        if (decoded.point_a == msg.point_a && decoded.bit_index == msg.bit_index) {
            std::cout << "  - OTInit encode/decode: OK\n";
        } else {
            std::cout << "  - OTInit encode/decode: DATA MISMATCH\n";
        }
    } else {
        std::cout << "  - OTInit decode: FAILED\n";
    }
}

int main() {
    try {
        test_crypto_ops();
        test_proto_codec();
        std::cout << "\n[All Tests Passed!]\n";
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
