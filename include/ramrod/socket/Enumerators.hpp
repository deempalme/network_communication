#ifndef RAMROD_SOCKET_ENUMERATORS_HPP
#define RAMROD_SOCKET_ENUMERATORS_HPP

#include <cstdint>  // For uint32_t


namespace ramrod::socket
{
    enum class ErrorType : std::uint8_t
    {
        /// @brief No error encountered
        SUCCESS = 0u,
        /// @brief hints.ai_flags contains invalid flags
        ADDRESS_INFO_BAD_FLAGS,
        /// @brief The requested address family is not supported
        ADDRESS_INFO_FAMILY_NOT_SUPPORTED,
        /// @brief The specified network host exists, but does not have any network addresses defined
        ADDRESS_INFO_NO_ADDRESS_DEFINED,
        /// @brief The node or service is not known
        ADDRESS_INFO_NO_NAME,
        /// @brief Out of memory
        ADDRESS_INFO_OUT_OF_MEMORY,
        /// @brief The name server returned a permanent failure indication
        ADDRESS_INFO_PERMANENT_FAILURE,
        /// @brief The requested service is not available for the requested socket type
        ADDRESS_INFO_SERVICE_NOT_AVAILABLE,
        /// @brief The requested socket type is not supported
        ADDRESS_INFO_SOCKET_TYPE_NOT_SUPPORTED,
        /// @brief Other system error; errno is set to indicate the error.
        ADDRESS_INFO_SYSTEM_ERROR,
        /// @brief The name server returned a temporary failure indication. Try again later
        ADDRESS_INFO_TRY_AGAIN_LATER,
        /// @brief The specified network host does not have any network addresses
        ADDRESS_INFO_UNKNOWN_ADDRESS_FAMILY,
        /// @brief There is already an active connection
        ALREADY_CONNECTED,
        /// @brief Error when binding socket
        BIND_SOCKET_ERROR,
        /// @brief Error when closing socket
        CLOSE_ERROR,
        /// @brief Error when creating socket
        CREATE_SOCKET_ERROR,
        /// @brief Failed signal connection failed to reap all dead processes
        DEAD_PROCESSES_REAPING_CONNECTION_FAILED,
        /// @brief Converting IP from number to string failed
        IP_CONVERSION_FAILED,
        /// @brief No socket is available with given parameters
        NO_SOCKET_AVAILABLE,
        /// @brief Error when setting socket options
        SET_SOCKET_OPTION_ERROR,
        /// @brief System error
        SYSTEM_ERROR,
    };

    enum class Family : std::uint8_t
    {
        /// @brief IP could be of any version
        UNSPECIFIED,
        /// @brief IP protocol family version 4 (32 bits)
        IPV4,
        /// @brief IP protocol family version 6 (128 bits)
        IPV6,
    };

    enum class SocketType : std::uint8_t
    {
        /// @brief (TCP) Sequenced, reliable, connection-based byte streams
        STREAM,
        /// @brief (UDP) Connectionless, unreliable datagrams of fixed maximum length
        DATAGRAM,
    };
} // namespace ramrod::socket

#endif // RAMROD_SOCKET_ENUMERATORS_HPP
