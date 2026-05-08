cmake_minimum_required(VERSION 3.16)
project(cot_server LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Networking integrated via platform-native APIs

set(COMMON_SOURCES
  src/crypto_ops.cpp
  src/ot_protocol.cpp
  src/proto_codec.cpp
  src/cot.pb.c
  external/nanopb/pb_common.c
  external/nanopb/pb_encode.c
  external/nanopb/pb_decode.c
  external/trezor-crypto/bignum.c
  external/trezor-crypto/ecdsa.c
  external/trezor-crypto/secp256k1.c
  external/trezor-crypto/curves.c
  external/trezor-crypto/sha2.c
  external/trezor-crypto/memzero.c
  external/trezor-crypto/base58.c
  external/trezor-crypto/address.c
  external/trezor-crypto/hasher.c
  external/trezor-crypto/ripemd160.c
  external/trezor-crypto/hmac.c
  external/trezor-crypto/sha3.c
  external/trezor-crypto/blake2b.c
  external/trezor-crypto/blake256.c
  external/trezor-crypto/groestl.c
  external/trezor-crypto/rand.c
  external/trezor-crypto/rfc6979.c
  external/trezor-crypto/hmac_drbg.c
)

set(COMMON_INCLUDES
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/external/nanopb
  ${CMAKE_CURRENT_SOURCE_DIR}/external/trezor-crypto
)

set_source_files_properties(src/cot.pb.c PROPERTIES LANGUAGE C)
set_source_files_properties(external/nanopb/pb_common.c PROPERTIES LANGUAGE C)
set_source_files_properties(external/nanopb/pb_encode.c PROPERTIES LANGUAGE C)
set_source_files_properties(external/nanopb/pb_decode.c PROPERTIES LANGUAGE C)

# Networking library for Windows
if(WIN32)
  set(NET_LIBS ws2_32)
endif()

add_executable(cot_server
  src/main.cpp
  src/server.cpp
  ${COMMON_SOURCES}
)
target_include_directories(cot_server PRIVATE ${COMMON_INCLUDES})
target_link_libraries(cot_server PRIVATE ${NET_LIBS})

# Standalone math check target
add_executable(math_check
  src/math_check.cpp
  ${COMMON_SOURCES}
)
target_include_directories(math_check PRIVATE ${COMMON_INCLUDES})
target_link_libraries(math_check PRIVATE ${NET_LIBS})
