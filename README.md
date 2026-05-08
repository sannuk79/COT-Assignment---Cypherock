# COT-MTA: Correlated OT-based Multiplicative-to-Additive Conversion

A secure two-party computation demo implementing Appendix A.3 style Correlated Oblivious Transfer for Multiplicative-to-Additive conversion over the secp256k1 scalar field.

## Table of Contents

- [Overview](#overview)
- [Protocol Summary](#protocol-summary)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Manual Testing](#manual-testing)
- [Implementation Details](#implementation-details)
- [Verification](#verification)
- [Security Notes](#security-notes)
- [Troubleshooting](#troubleshooting)
- [Current Repository Status](#current-repository-status)

## Overview

This project demonstrates Multiplicative-to-Additive (MTA) share conversion using Correlated Oblivious Transfer (COT).

Two parties start with multiplicative shares:

- Alice (client) holds `x`
- Bob (server) holds `y`

The goal is to compute additive shares `U` and `V` such that:

```text
x * y mod n = U + V mod n
```

where `n` is the secp256k1 curve order.

Neither party should need to reveal its multiplicative share to the other during the protocol.

## Protocol Summary

For each bit `y_i` of Bob's 256-bit scalar `y`:

| Step | Alice (Client) | Bob (Server) |
| --- | --- | --- |
| 1 | Sample random scalar `U_i` | - |
| 2 | Set `m0 = U_i`, `m1 = U_i + x mod n` | - |
| 3 | Run 1-out-of-2 OT | Use choice bit `c = y_i` |
| 4 | - | Receive `mc_i = U_i + y_i * x mod n` |

After all 256 rounds:

```text
U = -Σ(2^i * U_i) mod n
V =  Σ(2^i * mc_i) mod n
```

Therefore:

```text
U + V = x * y mod n
```

## Project Structure

```text
cot-mta/
├── proto/
│   └── cot.proto
├── server/
│   ├── CMakeLists.txt
│   ├── external/
│   └── src/
│       ├── main.cpp
│       ├── crypto_ops.h
│       ├── crypto_ops.cpp
│       ├── ot_protocol.h
│       ├── ot_protocol.cpp
│       ├── server.h
│       └── server.cpp
├── client/
│   ├── package.json
│   ├── tsconfig.json
│   ├── proto/
│   └── src/
│       ├── client.ts
│       ├── crypto_ops.ts
│       └── ot_protocol.ts
└── README.md
```

## Prerequisites

### Client

- Node.js 18+ recommended
- npm

Client dependencies are installed through `npm install`.

### Server

- CMake 3.16+
- C++17 compiler (e.g., MinGW-w64, GCC, Clang)
- No external heavy dependencies (Boost is no longer required)

## Quick Start

### Option 1: Local Demo in TypeScript

This is the working path in the current repository. It simulates the full COT/MTA math locally and verifies:

```text
U + V mod n == x * y mod n
```

Run:

```bash
cd client
npm install
npm run build
npm start
```

This default mode generates random `x` and `y`.

### Example Output

```text
[Client] multiplicative share x = 0x...
[Client] multiplicative share y = 0x...
[Client] additive share U      = 0x...
[Client] additive share V      = 0x...
[Client] verification         = ok
```

### Option 2: Network Demo with C++ Server

The repository now includes a simple end-to-end network path:

- the C++ server generates Bob's multiplicative share `y`
- the TypeScript client connects over TCP
- both sides run the 256 OT rounds
- each side prints its own multiplicative/additive shares locally

Build the client first:

```bash
cd client
npm install
npm run build
```

Build the server from the repo root:

```bash
cmake -S server -B server/build
cmake --build server/build --config Release
```

Run the server in one terminal:

```bash
server/build/cot_server 8888
```

or on Windows with the `.exe` name:

```bash
server\build\cot_server.exe 8888
```

Then run the client in a second terminal:

```bash
cd client
npm start -- --host 127.0.0.1 --port 8888
```

You can also provide a fixed Alice share:

```bash
npm start -- --host 127.0.0.1 --port 8888 --x 01
```

Notes:

- If `--host` is provided, the client switches from local mode to network mode.
- In network mode, `y` is generated and printed only on the server side.
- In network mode, the client must not be started with `--y`.
- The server currently handles one connection/session and then exits.
- Network mode intentionally does not reveal Bob's secret `y` back to the client, so full verification output is unavailable there unless you add an explicit debug-only reveal path.

## Manual Testing

The client supports manual scalar input for deterministic testing.

### Commands

From `client/`:

```bash
node dist/client.js --x 01 --y 02
node dist/client.js --x 1234abcd --y deadbeef
node dist/client.js --x a1b2c3d4e5f67890 --y 9988776655443322
```

You can also use the npm script:

```bash
npm start -- --x 01 --y 02
npm start -- --x 1234abcd --y deadbeef
```

### Two Simple Examples

Example 1:

```bash
node dist/client.js --x 01 --y 02
```

Meaning:

- `x = 1`
- `y = 2`

Example 2:

```bash
node dist/client.js --x 1234abcd --y deadbeef
```

Meaning:

- `x = 0x1234abcd`
- `y = 0xdeadbeef`

### Input Rules

- Hex input only
- `0x` prefix is optional
- Maximum input size is 32 bytes
- Value must be in range `[1, n-1]`
- Short inputs are left-padded internally to 32 bytes

### Help

```bash
node dist/client.js --help
```

## Implementation Details

### TypeScript Client

Files:

- [client.ts](D:/otcrypto/client/src/client.ts:1)
- [crypto_ops.ts](D:/otcrypto/client/src/crypto_ops.ts:1)
- [ot_protocol.ts](D:/otcrypto/client/src/ot_protocol.ts:1)

Highlights:

- Uses `@noble/secp256k1` for point arithmetic
- Uses BigInt for scalar arithmetic modulo secp256k1 order
- Computes `U` and `V` from 256 OT rounds
- Supports random and manual test inputs
- Prints verification result clearly

### Implementation Architecture

The C++ server has been re-engineered for maximum portability and performance:

1.  **Cryptographic Core (`trezor-crypto`)**: 
    - Full removal of Boost Multiprecision.
    - Optimized 256-bit modular arithmetic using 30-bit limbs.
    - Constant-time ECC primitives for `secp256k1`.
2.  **Serialization (`nanopb`)**:
    - Migration from manual byte-streams to structured Protocol Buffers.
    - Zero-allocation runtime, ideal for performance-critical systems.
    - Custom-bound message descriptors for the `COT` schema.
3.  **Networking (Native Sockets)**:
    - Lightweight `TcpSocket` implementation replacing Boost.Asio.
    - Unified interface for Windows (`WinSock2`) and Linux (`POSIX`).
    - Framed message transport (4-byte length prefix + payload).

### Protocol Flow (Deep Dive)

The protocol implements a Correlated OT variant to achieve MTA:

1.  **Base OT Preparation**: Alice (Client) initializes 256 rounds of OT. For each bit, she derives encryption keys from the X-coordinate of a shared elliptic curve point.
2.  **Encryption**: Alice encrypts two potential shares ($U_i$ and $U_i + x$) using XOR masking derived from the shared secret.
3.  **Selection**: Bob (Server) selects the appropriate share based on his choice bit $y_i$ (the $i$-th bit of his secret $y$).
4.  **Aggregation**: Both parties aggregate their shares using modular summation. Alice's final share is $U = -\sum 2^i U_i \pmod{n}$, and Bob's is $V = \sum 2^i mc_i \pmod{n}$.

### Repository Status

- [x] **Verified Math**: `math_check` confirms 100% correctness of the trezor-crypto integration.
- [x] **Minimal Footprint**: Binary size significantly reduced by removing Boost.
- [x] **Portable**: Compiles cleanly with MinGW-w64 on Windows and GCC on Linux.

### Protocol Buffers

Shared schema:

- [cot.proto](D:/otcrypto/proto/cot.proto:1)

```proto
syntax = "proto3";

message OTInit {
  bytes point_a = 1;
  uint32 bit_index = 2;
}

message OTResponse {
  bytes point_b = 1;
}

message OTEncrypted {
  bytes encrypted_m0 = 1;
  bytes encrypted_m1 = 2;
  bytes nonce = 3;
}

message MTAShare {
  bytes additive_share = 1;
}
```

## Verification

The local client demo verifies:

```text
(x * y) mod n == (U + V) mod n
```

The current console output format is:

```text
[Client] multiplicative share x = 0x...
[Client] multiplicative share y = 0x...
[Client] additive share U      = 0x...
[Client] additive share V      = 0x...
[Client] verification         = ok
```

If the protocol math is correct, verification prints `ok`.

For the network demo, the client does not receive Bob's secret multiplicative share `y`, so the client cannot perform the final `x * y == U + V` check without adding a separate debug-only reveal path.

## Security Notes

This repository is a demo and should not be treated as production-secure.

Current limitations:

- XOR masking is used in the demo flow instead of authenticated encryption
- No malicious security protections
- No session management or replay protection
- No TLS or authenticated transport
- No constant-time hardening guarantees in the demo code

For production-style hardening, consider:

- AES-256-GCM or ChaCha20-Poly1305
- HKDF-based key derivation with context separation
- Mutual authentication and TLS
- Constant-time scalar and point handling

## Troubleshooting

### `npm install` fails at repo root

Use the client directory:

```bash
cd client
npm install
```

or:

```bash
npm --prefix client install
```

### TypeScript build errors from old noble helpers

This repo already uses local helper functions instead of deprecated `secp.utils.bytesToNumberBE` and `numberToBytesBE`.

Rebuild with:

```bash
cd client
npm run build
```

### Verification shows `failed`

Check:

- bit extraction order
- scalar arithmetic is modulo secp256k1 order
- manual input values are within valid range

### Client cannot connect to server

Check:

- the server is already running before starting the client
- the client is using `--host 127.0.0.1 --port 8888`
- the built server binary exists under `server/build`
- the chosen port is not blocked or already in use

### C++ build is slow or appears stuck during configure

The first `cmake -S server -B server/build` run may take time while the toolchain is detected.

If needed, remove the partial build directory and configure again:

```bash
rmdir /s /q server\build
cmake -S server -B server/build
```

## Current Repository Status

What is working now:

- TypeScript local demo
- TypeScript client network mode with `--host` and `--port`
- C++ server TCP listener and framed message flow
- random input mode
- manual input mode with `--x` and `--y` for local testing
- verification output

What is scaffolded but incomplete:

- authenticated encryption instead of XOR masking
- repeated multi-session server lifecycle
- generated protobuf bindings for both sides

## Implementation Details (Server Stack)

The server has been migrated to a low-footprint, portable stack:

- **nanopb**: Used for memory-efficient protobuf serialization.
- **trezor-crypto**: Used for all cryptographic primitives (Bignum, Secp256k1, SHA256).
- **Native Sockets**: Replaced Boost.Asio with a minimal socket wrapper for TCP communication.

## Verification

### Standalone Math Check
The server includes a dedicated verification tool to ensure mathematical correctness:
```bash
server/build/math_check
```
This tool verifies:
- Scalar modular addition/subtraction/multiplication.
- ECC Point addition and scalar multiplication.
- Nanopb encoding/decoding consistency.

### Network Demo
(Existing network verification notes apply)
