#include "server.h"

#include "proto_codec.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
#ifdef _WIN32
struct WinSockInit {
    WinSockInit() {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }
    ~WinSockInit() { WSACleanup(); }
} winsock_init;
#endif

class Socket {
public:
    explicit Socket(SOCKET s) : s_(s) {}
    ~Socket() { if (s_ != INVALID_SOCKET) closesocket(s_); }
    
    void send(const void* data, size_t size) {
        const char* ptr = static_cast<const char*>(data);
        while (size > 0) {
            int sent = ::send(s_, ptr, static_cast<int>(size), 0);
            if (sent == SOCKET_ERROR) throw std::runtime_error("send failed");
            ptr += sent;
            size -= sent;
        }
    }
    
    void receive(void* data, size_t size) {
        char* ptr = static_cast<char*>(data);
        while (size > 0) {
            int recvd = ::recv(s_, ptr, static_cast<int>(size), 0);
            if (recvd <= 0) throw std::runtime_error("receive failed or connection closed");
            ptr += recvd;
            size -= recvd;
        }
    }

    SOCKET get() const { return s_; }

private:
    SOCKET s_;
};

uint8_t get_bit(const Scalar& scalar, size_t bit_index) {
  const uint8_t byte = scalar[scalar.size() - 1 - (bit_index / 8)];
  return static_cast<uint8_t>((byte >> (bit_index % 8)) & 0x01);
}

std::vector<uint8_t> read_framed(Socket& socket) {
  std::array<uint8_t, 4> header{};
  socket.receive(header.data(), 4);

  const uint32_t size = (static_cast<uint32_t>(header[0]) << 24) |
                        (static_cast<uint32_t>(header[1]) << 16) |
                        (static_cast<uint32_t>(header[2]) << 8) |
                        static_cast<uint32_t>(header[3]);

  std::vector<uint8_t> body(size);
  if (size > 0) {
    socket.receive(body.data(), size);
  }
  return body;
}

void write_framed(Socket& socket, const std::vector<uint8_t>& body) {
  const uint32_t size = static_cast<uint32_t>(body.size());
  std::array<uint8_t, 4> header = {
      static_cast<uint8_t>((size >> 24) & 0xFF),
      static_cast<uint8_t>((size >> 16) & 0xFF),
      static_cast<uint8_t>((size >> 8) & 0xFF),
      static_cast<uint8_t>(size & 0xFF)};

  socket.send(header.data(), 4);
  if (size > 0) {
    socket.send(body.data(), size);
  }
}
}  // namespace

ServerApp::ServerApp(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port) {}

void ServerApp::run() {
  SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == INVALID_SOCKET) throw std::runtime_error("failed to create socket");
  
  Socket listener(listen_sock);
  
  int opt = 1;
  setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_);
  inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);

  if (bind(listen_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
    throw std::runtime_error("bind failed");
  }

  if (listen(listen_sock, 1) == SOCKET_ERROR) {
    throw std::runtime_error("listen failed");
  }

  std::cout << "[Server] listening on " << host_ << ":" << port_ << "\n";

  sockaddr_in client_addr{};
  int client_addr_len = sizeof(client_addr);
  SOCKET client_sock = accept(listen_sock, (sockaddr*)&client_addr, &client_addr_len);
  if (client_sock == INVALID_SOCKET) throw std::runtime_error("accept failed");

  Socket socket(client_sock);
  std::cout << "[Server] client connected\n";

  SessionConfigMessage session_config{};
  const auto config_bytes = read_framed(socket);
  if (!ProtoCodec::decode_session_config(config_bytes, session_config)) {
    throw std::runtime_error("failed to decode SessionConfig");
  }

  const Scalar y = CryptoOps::generate_random_scalar();
  std::cout << "[Server] multiplicative share y = 0x" << CryptoOps::hex_encode(y) << "\n";

  std::array<Scalar, 256> mc_values{};

  for (size_t bit_idx = 0; bit_idx < 256; ++bit_idx) {
    OTInitMessage init{};
    const auto init_bytes = read_framed(socket);
    if (!ProtoCodec::decode_ot_init(init_bytes, init)) {
      throw std::runtime_error("failed to decode OTInit");
    }
    if (init.bit_index != bit_idx) {
      throw std::runtime_error("unexpected bit index from client");
    }

    BobOT bob(get_bit(y, bit_idx));
    const auto maybe_b = bob.process_init(init.point_a);
    if (!maybe_b) {
      throw std::runtime_error("failed to process OT init");
    }

    write_framed(socket, ProtoCodec::encode_ot_response(OTResponseMessage{*maybe_b}));

    OTEncryptedPayload encrypted{};
    const auto encrypted_bytes = read_framed(socket);
    if (!ProtoCodec::decode_ot_encrypted(encrypted_bytes, encrypted)) {
      throw std::runtime_error("failed to decode OTEncrypted");
    }

    if (!bob.decrypt_message(encrypted, mc_values[bit_idx])) {
      throw std::runtime_error("failed to decrypt OT payload");
    }
  }

  const Scalar v = MTA::compute_bob_share(y, mc_values);
  write_framed(socket, ProtoCodec::encode_scalar(ScalarMessage{v}));

  std::cout << "[Server] additive share V      = 0x" << CryptoOps::hex_encode(v) << "\n";

  if (session_config.debug_reveal) {
    DebugRevealRequestMessage debug_request{};
    const auto debug_request_bytes = read_framed(socket);
    if (!ProtoCodec::decode_debug_reveal_request(debug_request_bytes, debug_request)) {
      throw std::runtime_error("failed to decode DebugRevealRequest");
    }

    const bool verified = MTA::verify_mta(debug_request.multiplicative_share_x, y,
                                          debug_request.additive_share_u, v);
    write_framed(socket, ProtoCodec::encode_debug_reveal_response(
                             DebugRevealResponseMessage{y, verified}));
    std::cout << "[Server] verification         = " << (verified ? "ok" : "failed") << "\n";
  }

  std::cout << "[Server] session complete\n";
}
