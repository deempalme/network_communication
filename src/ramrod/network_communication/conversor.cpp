#include "ramrod/network_communication/conversor.h"

#include <netinet/in.h>  // for htonl, htons, ntohl, ntohs

namespace ramrod {
  namespace network_communication {
    std::uint16_t conversor::host_to_network(const std::uint16_t host_value){
      return ::htons(host_value);
    }

    std::uint32_t conversor::host_to_network(const std::uint32_t host_value){
      return ::htonl(host_value);
    }

    std::uint16_t conversor::network_to_host(const std::uint16_t network_value){
      return ::ntohs(network_value);
    }

    std::uint32_t conversor::network_to_host(const std::uint32_t network_value){
      return ::ntohl(network_value);
    }
  } // namespace: network_communication 
} // namespace: ramrod