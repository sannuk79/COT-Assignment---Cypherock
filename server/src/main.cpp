#include "server.h"

#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  std::string host = "0.0.0.0";
  uint16_t port = 8888;

  if (argc == 2) {
    port = static_cast<uint16_t>(std::stoi(argv[1]));
  } else if (argc >= 3) {
    host = argv[1];
    port = static_cast<uint16_t>(std::stoi(argv[2]));
  }

  try {
    ServerApp app(host, port);
    app.run();
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "fatal: " << ex.what() << "\n";
    return 1;
  }
}
