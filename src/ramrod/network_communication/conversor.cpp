#include "ramrod/network_communication/conversor.h"

#include <netinet/in.h>  // for htonl, htons, ntohl, ntohs

namespace ramrod::network_communication {
  std::uint16_t conversor::host_to_network(const std::uint16_t host_value){
    return ::htons(host_value);
  }

  std::uint32_t conversor::host_to_network(const std::uint32_t host_value){
    return ::htonl(host_value);
  }

  std::uint64_t conversor::host_to_network(const std::uint64_t host_value){
    union {
      std::uint64_t result;
      std::uint8_t bytes[8];
    };

    bytes[0] = (host_value & 0x00000000000000ff);
    bytes[1] = (host_value & 0x000000000000ff00) >> 8;
    bytes[2] = (host_value & 0x0000000000ff0000) >> 16;
    bytes[3] = (host_value & 0x00000000ff000000) >> 24;
    bytes[4] = (host_value & 0x000000ff00000000) >> 32;
    bytes[5] = (host_value & 0x0000ff0000000000) >> 40;
    bytes[6] = (host_value & 0x00ff000000000000) >> 48;
    bytes[7] = (host_value & 0xff00000000000000) >> 56;

    return result;
  }

  std::uint16_t conversor::network_to_host(const std::uint16_t network_value){
    return ::ntohs(network_value);
  }

  std::uint32_t conversor::network_to_host(const std::uint32_t network_value){
    return ::ntohl(network_value);
  }

  std::uint64_t conversor::network_to_host(const std::uint64_t network_value){
    union {
      std::uint64_t input;
      std::uint8_t bytes[8];
    };

    input = network_value;

    return bytes[0] |
        ((std::uint64_t)bytes[1]) <<  8 |
        ((std::uint64_t)bytes[2]) << 16 |
        ((std::uint64_t)bytes[3]) << 24 |
        ((std::uint64_t)bytes[4]) << 32 |
        ((std::uint64_t)bytes[5]) << 40 |
        ((std::uint64_t)bytes[6]) << 48 |
        ((std::uint64_t)bytes[7]) << 56;
  }
} // namespace ramrod::network_communication
