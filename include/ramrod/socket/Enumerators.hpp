#ifndef RAMROD_SOCKET_ENUMERATORS_HPP
#define RAMROD_SOCKET_ENUMERATORS_HPP

#include <cstdint>  // For uint32_t


namespace ramrod::socket
{
    enum class ErrorType : std::uint8_t
    {
        /// @brief No error encountered
        SUCCESS = 0u,
        /// @brief Error when binding socket
        BIND_SOCKET_ERROR,
        /// @brief Error when creating socket
        CREATE_SOCKET_ERROR,
        /// @brief Error when getting address info
        GET_ADDRESS_INFO_ERROR,
        /// @brief Error when setting socket options
        SET_SOCKET_OPTION_ERROR,
    };

    enum class Protocol : std::uint8_t
    {
        /// @brief IP protocol family version 4 (32 bits)
        IPV4,
        /// @brief IP protocol family version 6 (128 bits)
        IPV6,
    };

    enum class SocketType : std::uint8_t
    {
        /// @brief (UDP) Connectionless, unreliable datagrams of fixed maximum length
        DATAGRAM,
        /// @brief (TCP) Sequenced, reliable, connection-based byte streams
        STREAM,
    };
} // namespace ramrod::socket

#endif // RAMROD_SOCKET_ENUMERATORS_HPP
