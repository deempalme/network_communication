#include "ramrod/socket/Conversor.hpp"

#include <cstdint>      // for uint64_t, uint32_t, uint16_t
#include <netinet/in.h> // for htonl, htons, ntohl, ntohs
#include <type_traits>  // for is_unsigned

namespace ramrod::socket
{
    template <>
    std::uint16_t Conversor::host_to_network(const std::uint16_t value)
    {
        return ::htons(value);
    }

    template <>
    std::uint32_t Conversor::host_to_network(const std::uint32_t value)
    {
        return ::htonl(value);
    }

    template <>
    std::uint64_t Conversor::host_to_network(const std::uint64_t value)
    {
        union
        {
            std::uint64_t result;
            std::uint8_t bytes[8];
        };

        bytes[0] = static_cast<std::uint8_t>(value & 0x00000000000000fful);
        bytes[1] = static_cast<std::uint8_t>((value & 0x000000000000ff00ul) >> 8u);
        bytes[2] = static_cast<std::uint8_t>((value & 0x0000000000ff0000ul) >> 16u);
        bytes[3] = static_cast<std::uint8_t>((value & 0x00000000ff000000ul) >> 24u);
        bytes[4] = static_cast<std::uint8_t>((value & 0x000000ff00000000ul) >> 32u);
        bytes[5] = static_cast<std::uint8_t>((value & 0x0000ff0000000000ul) >> 40u);
        bytes[6] = static_cast<std::uint8_t>((value & 0x00ff000000000000ul) >> 48u);
        bytes[7] = static_cast<std::uint8_t>((value & 0xff00000000000000ul) >> 56u);

        return result;
    }

    template <typename T>
    T Conversor::host_to_network(const T value)
    {
        static_assert(std::is_unsigned<T>::value(),
                      "host_to_network() only accepts unsigned integers");

        return host_to_network<T>(value);
    }

    template <>
    std::uint16_t Conversor::network_to_host(const std::uint16_t value)
    {
        return ::ntohs(value);
    }

    template <>
    std::uint32_t Conversor::network_to_host(const std::uint32_t value)
    {
        return ::ntohl(value);
    }

    template <>
    std::uint64_t Conversor::network_to_host(const std::uint64_t value)
    {
        union
        {
            std::uint64_t input;
            std::uint8_t bytes[8];
        };

        input = value;

        return bytes[0] |
               static_cast<std::uint64_t>(bytes[1]) << 8u |
               static_cast<std::uint64_t>(bytes[2]) << 16u |
               static_cast<std::uint64_t>(bytes[3]) << 24u |
               static_cast<std::uint64_t>(bytes[4]) << 32u |
               static_cast<std::uint64_t>(bytes[5]) << 40u |
               static_cast<std::uint64_t>(bytes[6]) << 48u |
               static_cast<std::uint64_t>(bytes[7]) << 56u;
    }

    template <typename T>
    T Conversor::network_to_host(const T value)
    {
        static_assert(std::is_unsigned<T>::value(),
                      "network_to_host() only accepts unsigned integers");

        return network_to_host<T>(value);
    }
} // namespace ramrod::network_communication
