#pragma once

#include "crypto_ops.h"
#include "ot_protocol.h"

#include <array>
#include <cstdint>
#include <string>

class ServerApp {
public:
  ServerApp(std::string host, uint16_t port);
  void run();

private:
  std::string host_;
  uint16_t port_;
};
