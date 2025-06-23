#ifndef RAMROD_SOCKET_ENUMERATORS_HPP
#define RAMROD_SOCKET_ENUMERATORS_HPP

#include <cstdint>  // For uint32_t


namespace ramrod::socket
{
    enum class ConnectStatus : std::uint8_t
    {
        /// @brief No error encountered
        SUCCESS = 0u,
        /// @brief Local address is already in use
        ADDRESS_ALREADY_IN_USE,
        /// @brief The bound address was found that all port numbers in range are currently in use
        ADDRESS_NOT_AVAILABLE,
        /// @brief Server socket is already open, only one is allowed per instance
        ALREADY_OPEN,
        /// @brief It is necessary to call \p connect() before at least once
        CONNECTION_HAS_NOT_BEEN_CALLED_YET,
        /// @brief The socket is nonblocking and the connection cannot be completed immediately
        CONNECTION_IN_PROGRESS,
        /// @brief No one found listening on the remote address
        CONNECTION_REFUSED,
        /// @brief Event has been interrupted by a signal
        INTERRUPTED_BY_A_SIGNAL,
        /// @brief The specified network host does not have any network addresses
        ///        in the requested address family. Or the node or service is not known
        INVALID_ADDRESS,
        /// @brief An I/O error occurred
        IO_ERROR,
        /// @brief IP address and port cannot be empty at the same time
        IP_AND_PORT_CANNOT_BE_EMPTY,
        /// @brief IP address and service cannot be empty at the same time
        IP_AND_SERVICE_CANNOT_BE_EMPTY,
        /// @brief The system-wide limit on the total number of open connections has
        ///        been reached
        MAXIMUM_CONNECTION_COUNT_REACHED,
        /// @brief Network is unreachable
        NETWORK_UNREACHABLE,
        /// @brief Permission to create connection with specified parameters was denied
        NO_ACCESS,
        /// @brief The specified network host exists, but does not have any network
        ///        addresses defined.
        NOT_OPEN,
        /// @brief Connection not open, or already disconnected
        NOT_CONNECTED,
        /// @brief Cannot call reopen() if open() has not been called at least once
        OPEN_HAS_NOT_BEEN_CALLED_YET,
        /// @brief The selected socket type does not support this operation
        OPERATION_NOT_SUPPORTED,
        /// @brief Out of memory
        OUT_OF_MEMORY,
        /// @brief The name server returned a permanent failure indication
        PERMANENT_FAILURE,
        /// @brief Permission denied to access socket file
        PERMISSION_DENIED,
        /// @brief Port cannot be zero/empty
        PORT_CANNOT_BE_EMPTY,
        /// @brief The protocol type or the specified protocol is not supported within
        ///        this domain/address
        PROTOCOL_NOT_SUPPORTED_BY_ADDRESS,
        /// @brief Queue is full, or its size was set to 0
        QUEUE_FULL,
        /// @brief Service string cannot be empty
        SERVICE_CANNOT_BE_EMPTY,
        /// @brief Other system error; errno is set to indicate the error
        SYSTEM_ERROR,
        /// @brief Timeout while attempting connection. The server may be too busy
        ///        to accept new connections
        TIMED_OUT,
        /// @brief The name server returned a temporary failure indication, or there
        ///        are insufficient entries in the routing cache, or a previous connection
        ///        attempt has not yet been completed. Try again later
        TRY_AGAIN_LATER,
        /// @brief Unknown error
        UNKNOWN_ERROR,
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

    enum class ReceiveStatus : std::uint8_t
    {
        /// @brief No error encountered when receiving data
        SUCCESS = 0u,
        /// @brief The receive buffer pointer(s) point outside the process's address space
        BUFFER_POINTER_FAULT,
        /// @brief The receive was interrupted by delivery of a signal before any data was available
        INTERRUPTED_BY_A_SIGNAL,
        /// @brief Invalid argument passed
        INVALID_ARGUMENT,
        /// @brief There is no connection to server
        NOT_CONNECTED,
        /// @brief Could not allocate memroy
        OUT_OF_MEMORY,
        /// @brief The connection is marked nonblocking and the receive operation would
        ///        block, or a receive timeout had been set and the timeout expired before
        ///        data was received
        TRY_RECEIVE_AGAIN,
        /// @brief Unknown error when receiving data
        UNKNOWN_ERROR,
    };

    enum class SendStatus : std::uint8_t
    {
        /// @brief No error encountered when sending data
        SUCCESS = 0u,
        /// @brief The socket type requires that message be sent atomically, and the
        ///        size of the message to be sent made this impossible
        BAD_MESSAGE_SIZE,
        /// @brief Connection reset by peer
        CONNECTION_RESET_BY_PEER,
        /// @brief A signal occurred before any data was transmitted
        INTERRUPTED_BY_A_SIGNAL,
        /// @brief Invalid argument passed
        INVALID_ARGUMENT,
        /// @brief Write permission is denied on the destination, or search permission
        ///        is denied for one of the directories the path prefix used in \p ip
        ///        when calling \p connect().
        ///
        ///        (For UDP sockets) An attempt was made to send to a network/broadcast
        ///        address as though it was a unicast address
        NO_ACCESS,
        /// @brief There is no connection to server
        NOT_CONNECTED,
        /// @brief No memory available
        OUT_OF_MEMORY,
        /// @brief The output queue for a network interface was full. This generally
        ///        indicates that the interface has stopped sending, but may be caused
        ///        by transient congestion.  (Normally, this does not occur in Linux.
        ///        Packets are just silently dropped when a device queue overflows)
        OUTPUT_QUEUE_FULL,
        /// @brief The connection is marked nonblocking and the requested operation would
        ///        block. Or connection has not been completed yet
        TRY_SEND_AGAIN,
        /// @brief Unknown error when sending data
        UNKNOWN_ERROR,
    };

    enum class ShutdownType : std::uint8_t
    {
        /// @brief Further receptions and transmissions will be disallowed
        ALL,
        /// @brief Further receptions will be disallowed
        RECEIVE,
        /// @brief  Further transmissions will be disallowed
        SEND,
    };
} // namespace ramrod::socket

#endif // RAMROD_SOCKET_ENUMERATORS_HPP
