#ifndef COT_PB_H_INCLUDED
#define COT_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current nanopb-generator.
#endif

#define SessionConfig_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, BOOL,     debug_reveal,      1)
#define SessionConfig_CALLBACK NULL
#define SessionConfig_DEFAULT NULL

#define OTInit_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, point_a,           1) \
X(a, STATIC,   SINGULAR, UINT32,   bit_index,         2)
#define OTInit_CALLBACK NULL
#define OTInit_DEFAULT NULL

#define OTResponse_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, point_b,           1)
#define OTResponse_CALLBACK NULL
#define OTResponse_DEFAULT NULL

#define OTEncrypted_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, encrypted_m0,      1) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, encrypted_m1,      2) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, nonce,             3)
#define OTEncrypted_CALLBACK NULL
#define OTEncrypted_DEFAULT NULL

#define MTAShare_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, additive_share,    1)
#define MTAShare_CALLBACK NULL
#define MTAShare_DEFAULT NULL

#define DebugRevealRequest_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, multiplicative_share_x,   1) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, additive_share_u,   2)
#define DebugRevealRequest_CALLBACK NULL
#define DebugRevealRequest_DEFAULT NULL

#define DebugRevealResponse_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FIXED_LENGTH_BYTES, multiplicative_share_y,   1) \
X(a, STATIC,   SINGULAR, BOOL,     verified,          2)
#define DebugRevealResponse_CALLBACK NULL
#define DebugRevealResponse_DEFAULT NULL

/* Struct definitions */
typedef struct _SessionConfig {
    bool debug_reveal;
} SessionConfig;

typedef struct _OTInit {
    pb_byte_t point_a[33];
    uint32_t bit_index;
} OTInit;

typedef struct _OTResponse {
    pb_byte_t point_b[33];
} OTResponse;

typedef struct _OTEncrypted {
    pb_byte_t encrypted_m0[32];
    pb_byte_t encrypted_m1[32];
    pb_byte_t nonce[12];
} OTEncrypted;

typedef struct _MTAShare {
    pb_byte_t additive_share[32];
} MTAShare;

typedef struct _DebugRevealRequest {
    pb_byte_t multiplicative_share_x[32];
    pb_byte_t additive_share_u[32];
} DebugRevealRequest;

typedef struct _DebugRevealResponse {
    pb_byte_t multiplicative_share_y[32];
    bool verified;
} DebugRevealResponse;

/* Initializer values for message structs */
#define SessionConfig_init_default               {0}
#define OTInit_init_default                      {{0}, 0}
#define OTResponse_init_default                  {{0}}
#define OTEncrypted_init_default                 {{0}, {0}, {0}}
#define MTAShare_init_default                    {{0}}
#define DebugRevealRequest_init_default          {{0}, {0}}
#define DebugRevealResponse_init_default         {{0}, 0}
#define SessionConfig_init_zero                  {0}
#define OTInit_init_zero                         {{0}, 0}
#define OTResponse_init_zero                     {{0}}
#define OTEncrypted_init_zero                    {{0}, {0}, {0}}
#define MTAShare_init_zero                       {{0}}
#define DebugRevealRequest_init_zero             {{0}, {0}}
#define DebugRevealResponse_init_zero            {{0}, 0}

/* Field tags (for use in manual encoding/decoding) */
#define SessionConfig_debug_reveal_tag           1
#define OTInit_point_a_tag                       1
#define OTInit_bit_index_tag                     2
#define OTResponse_point_b_tag                   1
#define OTEncrypted_encrypted_m0_tag             1
#define OTEncrypted_encrypted_m1_tag             2
#define OTEncrypted_nonce_tag                    3
#define MTAShare_additive_share_tag              1
#define DebugRevealRequest_multiplicative_share_x_tag 1
#define DebugRevealRequest_additive_share_u_tag  2
#define DebugRevealResponse_multiplicative_share_y_tag 1
#define DebugRevealResponse_verified_tag         2

#ifdef __cplusplus
extern "C" {
#endif

/* Struct field encoding specification for nanopb */
extern const pb_msgdesc_t SessionConfig_msg;
extern const pb_msgdesc_t OTInit_msg;
extern const pb_msgdesc_t OTResponse_msg;
extern const pb_msgdesc_t OTEncrypted_msg;
extern const pb_msgdesc_t MTAShare_msg;
extern const pb_msgdesc_t DebugRevealRequest_msg;
extern const pb_msgdesc_t DebugRevealResponse_msg;

#ifdef __cplusplus
}
#endif

/* Defines for backwards compatibility with older nanopb */
#define SessionConfig_fields &SessionConfig_msg
#define OTInit_fields &OTInit_msg
#define OTResponse_fields &OTResponse_msg
#define OTEncrypted_fields &OTEncrypted_msg
#define MTAShare_fields &MTAShare_msg
#define DebugRevealRequest_fields &DebugRevealRequest_msg
#define DebugRevealResponse_fields &DebugRevealResponse_msg

/* Maximum encoded size of messages (where known) */
#define SessionConfig_size                       2
#define OTInit_size                              41
#define OTResponse_size                          35
#define OTEncrypted_size                         80
#define MTAShare_size                            34
#define DebugRevealRequest_size                  68
#define DebugRevealResponse_size                 36

#endif
