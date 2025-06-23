#include "ramrod/socket/ErrorHandler.hpp"

#include <cerrno>  // for EBADF, EINVAL, ENOTSOCK, ENOMEM, EINTR, ENOBUFS
#include <netdb.h> // for EAI_ADDRFAMILY, EAI_AGAIN, EAI_BADFLAGS, EAI_FAIL

namespace
{
    /// @brief Value that indicates an error when receiving or sending data
    static constexpr ssize_t TRANSFER_ERROR{-1l};
    /// @brief Unknown error message
    static constexpr char UNKNOWN_ERROR_MSG[]{"Unknown error"};
} // Unnamed namespace

namespace ramrod::socket
{
    const char *get_status_detail(const ConnectStatus status)
    {
        switch (status)
        {
        case ConnectStatus::SUCCESS:
            static constexpr char SUCCESS_MSG[]{"No error encountered"};
            return SUCCESS_MSG;
        case ConnectStatus::ADDRESS_ALREADY_IN_USE:
            static constexpr char ADDRESS_ALREADY_IN_USE_MSG[]{"Local address is already in use"};
            return ADDRESS_ALREADY_IN_USE_MSG;
        case ConnectStatus::ADDRESS_NOT_AVAILABLE:
            static constexpr char ADDRESS_NOT_AVAILABLE_MSG[]{
                "he bound address was found that all port numbers in range are currently in use"};
            return ADDRESS_NOT_AVAILABLE_MSG;
        case ConnectStatus::ALREADY_OPEN:
            static constexpr char ALREADY_OPEN_MSG[]{
                "Server socket is already open, only one is allowed per instance"};
            return ALREADY_OPEN_MSG;
        case ConnectStatus::CONNECTION_HAS_NOT_BEEN_CALLED_YET:
            static constexpr char CONNECTION_HAS_NOT_BEEN_CALLED_YET_MSG[]{
                "It is necessary to call connect() before at least once"};
            return CONNECTION_HAS_NOT_BEEN_CALLED_YET_MSG;
        case ConnectStatus::CONNECTION_IN_PROGRESS:
            static constexpr char CONNECTION_IN_PROGRESS_MSG[]{
                "The socket is nonblocking and the connection cannot be completed immediately"};
            return CONNECTION_IN_PROGRESS_MSG;
        case ConnectStatus::CONNECTION_REFUSED:
            static constexpr char CONNECTION_REFUSED_MSG[]{
                "No one found listening on the remote address"};
            return CONNECTION_REFUSED_MSG;
        case ConnectStatus::INTERRUPTED_BY_A_SIGNAL:
            static constexpr char INTERRUPTED_BY_A_SIGNAL_MSG[]{
                "Event has been interrupted by a signal"};
            return INTERRUPTED_BY_A_SIGNAL_MSG;
        case ConnectStatus::INVALID_ADDRESS:
            static constexpr char INVALID_ADDRESS_MSG[]{
                "The specified network host does not have any network addresses "
                "in the requested address family. Or the node or service is not known"};
            return INVALID_ADDRESS_MSG;
        case ConnectStatus::IO_ERROR:
            static constexpr char IO_ERROR_MSG[]{"An I/O error occurred"};
            return IO_ERROR_MSG;
        case ConnectStatus::IP_AND_PORT_CANNOT_BE_EMPTY:
            static constexpr char IP_AND_PORT_CANNOT_BE_EMPTY_MSG[]{
                "IP address and port cannot be empty at the same time"};
            return IP_AND_PORT_CANNOT_BE_EMPTY_MSG;
        case ConnectStatus::IP_AND_SERVICE_CANNOT_BE_EMPTY:
            static constexpr char IP_AND_SERVICE_CANNOT_BE_EMPTY_MSG[]{
                "IP address and service cannot be empty at the same time"};
            return IP_AND_SERVICE_CANNOT_BE_EMPTY_MSG;
        case ConnectStatus::MAXIMUM_CONNECTION_COUNT_REACHED:
            static constexpr char MAXIMUM_CONNECTION_COUNT_REACHED_MSG[]{
                "The system-wide limit on the total number of open connections has "
                "been reached"};
            return MAXIMUM_CONNECTION_COUNT_REACHED_MSG;
        case ConnectStatus::NETWORK_UNREACHABLE:
            static constexpr char NETWORK_UNREACHABLE_MSG[]{"Network is unreachable"};
            return NETWORK_UNREACHABLE_MSG;
        case ConnectStatus::NO_ACCESS:
            static constexpr char NO_ACCESS_MSG[]{
                "Permission to create connection with specified parameters was denied"};
            return NO_ACCESS_MSG;
        case ConnectStatus::NOT_OPEN:
            static constexpr char NOT_OPEN_MSG[]{
                "The specified network host exists, but does not have any network "
                "addresses defined."};
            return NOT_OPEN_MSG;
        case ConnectStatus::NOT_CONNECTED:
            static constexpr char NOT_CONNECTED_MSG[]{
                "Connection not open, or already disconnected"};
            return NOT_CONNECTED_MSG;
        case ConnectStatus::OPEN_HAS_NOT_BEEN_CALLED_YET:
            static constexpr char OPEN_HAS_NOT_BEEN_CALLED_YET_MSG[]{
                "Cannot call reopen() if open() has not been called at least once"};
            return OPEN_HAS_NOT_BEEN_CALLED_YET_MSG;
        case ConnectStatus::OPERATION_NOT_SUPPORTED:
            static constexpr char OPERATION_NOT_SUPPORTED_MSG[]{
                "The selected socket type does not support this operation"};
            return OPERATION_NOT_SUPPORTED_MSG;
        case ConnectStatus::OUT_OF_MEMORY:
            static constexpr char OUT_OF_MEMORY_MSG[]{"Out of memory"};
            return OUT_OF_MEMORY_MSG;
        case ConnectStatus::PERMANENT_FAILURE:
            static constexpr char PERMANENT_FAILURE_MSG[]{
                "The name server returned a permanent failure indication"};
            return PERMANENT_FAILURE_MSG;
        case ConnectStatus::PERMISSION_DENIED:
            static constexpr char PERMISSION_DENIED_MSG[]{
                "Permission denied to access socket file"};
            return PERMISSION_DENIED_MSG;
        case ConnectStatus::PORT_CANNOT_BE_EMPTY:
            static constexpr char PORT_CANNOT_BE_EMPTY_MSG[]{"Port cannot be zero/empty"};
            return PORT_CANNOT_BE_EMPTY_MSG;
        case ConnectStatus::PROTOCOL_NOT_SUPPORTED_BY_ADDRESS:
            static constexpr char PROTOCOL_NOT_SUPPORTED_BY_ADDRESS_MSG[]{
                "The protocol type or the specified protocol is not supported within "
                "this domain/address"};
            return PROTOCOL_NOT_SUPPORTED_BY_ADDRESS_MSG;
        case ConnectStatus::QUEUE_FULL:
            static constexpr char QUEUE_FULL_MSG[]{"Queue is full, or its size was set to 0"};
            return QUEUE_FULL_MSG;
        case ConnectStatus::SERVICE_CANNOT_BE_EMPTY:
            static constexpr char SERVICE_CANNOT_BE_EMPTY_MSG[]{"Service string cannot be empty"};
            return SERVICE_CANNOT_BE_EMPTY_MSG;
        case ConnectStatus::SYSTEM_ERROR:
            static constexpr char SYSTEM_ERROR_MSG[]{
                "Other system error; errno is set to indicate the error"};
            return SYSTEM_ERROR_MSG;
        case ConnectStatus::TIMED_OUT:
            static constexpr char TIMED_OUT_MSG[]{
                "Timeout while attempting connection. The server may be too busy "
                "to accept new connections"};
            return TIMED_OUT_MSG;
        case ConnectStatus::TRY_AGAIN_LATER:
            static constexpr char TRY_AGAIN_LATER_MSG[]{
                "The name server returned a temporary failure indication, or there "
                "are insufficient entries in the routing cache, or a previous connection "
                "attempt has not yet been completed. Try again later"};
            return TRY_AGAIN_LATER_MSG;
        default:
        case ConnectStatus::UNKNOWN_ERROR:
            return UNKNOWN_ERROR_MSG;
        }
    }

    const char *get_status_detail(const ReceiveStatus status)
    {
        switch (status)
        {
        case ReceiveStatus::SUCCESS:
            static constexpr char SUCCESS_MSG[]{"No error encountered when receiving data"};
            return SUCCESS_MSG;
        case ReceiveStatus::BUFFER_POINTER_FAULT:
            static constexpr char BUFFER_POINTER_FAULT_MSG[]{
                "The receive buffer pointer(s) point outside the process's address space"};
            return BUFFER_POINTER_FAULT_MSG;
        case ReceiveStatus::INTERRUPTED_BY_A_SIGNAL:
            static constexpr char INTERRUPTED_BY_A_SIGNAL_MSG[]{
                "The receive was interrupted by delivery of a signal before any data was available"};
            return INTERRUPTED_BY_A_SIGNAL_MSG;
        case ReceiveStatus::INVALID_ARGUMENT:
            static constexpr char INVALID_ARGUMENT_MSG[]{"Invalid argument passed"};
            return INVALID_ARGUMENT_MSG;
        case ReceiveStatus::NOT_CONNECTED:
            static constexpr char NOT_CONNECTED_MSG[]{"There is no connection to server"};
            return NOT_CONNECTED_MSG;
        case ReceiveStatus::OUT_OF_MEMORY:
            static constexpr char OUT_OF_MEMORY_MSG[]{"Could not allocate memroy"};
            return OUT_OF_MEMORY_MSG;
        case ReceiveStatus::TRY_RECEIVE_AGAIN:
            static constexpr char TRY_RECEIVE_AGAIN_MSG[]{
                "The connection is marked nonblocking and the receive operation would "
                "block, or a receive timeout had been set and the timeout expired before "
                "data was received"};
            return TRY_RECEIVE_AGAIN_MSG;
        default:
        case ReceiveStatus::UNKNOWN_ERROR:
            return UNKNOWN_ERROR_MSG;
        }
    }

    const char *get_status_detail(const SendStatus status)
    {
        switch (status)
        {
        case SendStatus::SUCCESS:
            static constexpr char SUCCESS_MSG[]{"No error encountered when sending data"};
            return SUCCESS_MSG;
        case SendStatus::BAD_MESSAGE_SIZE:
            static constexpr char BAD_MESSAGE_SIZE_MSG[]{
                "The socket type requires that message be sent atomically, and the "
                "size of the message to be sent made this impossible"};
            return BAD_MESSAGE_SIZE_MSG;
        case SendStatus::CONNECTION_RESET_BY_PEER:
            static constexpr char CONNECTION_RESET_BY_PEER_MSG[]{"Connection reset by peer"};
            return CONNECTION_RESET_BY_PEER_MSG;
        case SendStatus::INTERRUPTED_BY_A_SIGNAL:
            static constexpr char INTERRUPTED_BY_A_SIGNAL_MSG[]{
                "A signal occurred before any data was transmitted"};
            return INTERRUPTED_BY_A_SIGNAL_MSG;
        case SendStatus::INVALID_ARGUMENT:
            static constexpr char INVALID_ARGUMENT_MSG[]{"Invalid argument passed"};
            return INVALID_ARGUMENT_MSG;
        case SendStatus::NO_ACCESS:
            static constexpr char NO_ACCESS_MSG[]{
                "Write permission is denied on the destination, or search permission "
                "is denied for one of the directories the path prefix used in ip "
                "when calling connect(). "
                "(For UDP sockets) An attempt was made to send to a network/broadcast "
                "address as though it was a unicast address"};
            return NO_ACCESS_MSG;
        case SendStatus::NOT_CONNECTED:
            static constexpr char NOT_CONNECTED_MSG[]{"There is no connection to server"};
            return NOT_CONNECTED_MSG;
        case SendStatus::OUT_OF_MEMORY:
            static constexpr char OUT_OF_MEMORY_MSG[]{"No memory available"};
            return OUT_OF_MEMORY_MSG;
        case SendStatus::OUTPUT_QUEUE_FULL:
            static constexpr char OUTPUT_QUEUE_FULL_MSG[]{
                "The output queue for a network interface was full. This generally "
                "indicates that the interface has stopped sending, but may be caused "
                "by transient congestion.  (Normally, this does not occur in Linux. "
                "Packets are just silently dropped when a device queue overflows)"};
            return OUTPUT_QUEUE_FULL_MSG;
        case SendStatus::TRY_SEND_AGAIN:
            static constexpr char TRY_SEND_AGAIN_MSG[]{
                "The connection is marked nonblocking and the requested operation would "
                "block. Or connection has not been completed yet"};
            return TRY_SEND_AGAIN_MSG;
        default:
        case SendStatus::UNKNOWN_ERROR:
            return UNKNOWN_ERROR_MSG;
        }
    }

    // ::::::::::::::::::::::::::::::::::: PROTECTED FUNCTIONS :::::::::::::::::::::::::::::::::::

    bool ErrorHandler::fill_receive_error(const int error_code,
                                          const ssize_t received_length,
                                          ReceiveStatus *status)
    {
        ReceiveStatus final_status{ReceiveStatus::SUCCESS};
        bool should_disconnect{};

        if (received_length == TRANSFER_ERROR)
        {
            switch (error_code)
            {
            case EAGAIN:
                final_status = ReceiveStatus::TRY_RECEIVE_AGAIN;
                break;
            case EBADF:
            case ECONNREFUSED:
            case ENOTCONN:
            case ENOTSOCK:
                final_status = ReceiveStatus::NOT_CONNECTED;
                should_disconnect = true;
                break;
            case EFAULT:
                final_status = ReceiveStatus::BUFFER_POINTER_FAULT;
                break;
            case EINTR:
                final_status = ReceiveStatus::INTERRUPTED_BY_A_SIGNAL;
                break;
            case EINVAL:
                final_status = ReceiveStatus::INVALID_ARGUMENT;
                break;
            case ENOMEM:
                final_status = ReceiveStatus::OUT_OF_MEMORY;
                break;
            default:
                final_status = ReceiveStatus::UNKNOWN_ERROR;
                break;
            }
        }

        /// Checking if \p status is not nullptr to make sure it can be filled
        if (status != nullptr)
            *status = final_status;

        return should_disconnect;
    }

    bool ErrorHandler::fill_send_error(const int error_code,
                                       const ssize_t sent_length,
                                       SendStatus *status)
    {
        SendStatus final_status{SendStatus::SUCCESS};
        bool should_disconnect{};

        if (sent_length == TRANSFER_ERROR)
        {
            switch (error_code)
            {
            case EACCES:
                final_status = SendStatus::NO_ACCESS;
                break;
            case EAGAIN:
                final_status = SendStatus::TRY_SEND_AGAIN;
                break;
            case EBADF:
            case EFAULT:
            case ENOTCONN:
            case ENOTSOCK:
            case EPIPE:
                final_status = SendStatus::NOT_CONNECTED;
                should_disconnect = true;
                break;
            case ECONNRESET:
                final_status = SendStatus::CONNECTION_RESET_BY_PEER;
                break;
            case EINTR:
                final_status = SendStatus::INTERRUPTED_BY_A_SIGNAL;
                break;
            case EINVAL:
                final_status = SendStatus::INVALID_ARGUMENT;
                break;
            case EMSGSIZE:
                final_status = SendStatus::BAD_MESSAGE_SIZE;
                break;
            case ENOBUFS:
                final_status = SendStatus::OUTPUT_QUEUE_FULL;
                break;
            case ENOMEM:
                final_status = SendStatus::OUT_OF_MEMORY;
                break;
            case EALREADY:
            case EDESTADDRREQ:
            case EISCONN:
            case EOPNOTSUPP:
                // Ignoring codes above
            default:
                final_status = SendStatus::UNKNOWN_ERROR;
                break;
            }
        }

        /// Checking if \p status is not nullptr to make sure it can be filled
        if (status != nullptr)
            *status = final_status;

        return should_disconnect;
    }

    ConnectStatus ErrorHandler::get_accept_error(const int error_code)
    {
        switch (error_code)
        {
        case EAGAIN:
            return ConnectStatus::TRY_AGAIN_LATER;
        case EBADF:
        case ENOTSOCK:
        case ECONNABORTED:
            return ConnectStatus::NOT_CONNECTED;
        case EINTR:
            return ConnectStatus::INTERRUPTED_BY_A_SIGNAL;
        case EINVAL:
            return ConnectStatus::NOT_OPEN;
        case EMFILE:
        case ENFILE:
            return ConnectStatus::MAXIMUM_CONNECTION_COUNT_REACHED;
        case ENOBUFS:
        case ENOMEM:
            return ConnectStatus::OUT_OF_MEMORY;
        case EOPNOTSUPP:
            return ConnectStatus::OPERATION_NOT_SUPPORTED;
        case EPROTO:
        // Ignoring errors above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_addr_info_error(const int error_code)
    {
        switch (error_code)
        {
        case EAI_ADDRFAMILY:
        case EAI_NONAME:
            return ConnectStatus::INVALID_ADDRESS;
        case EAI_AGAIN:
            return ConnectStatus::TRY_AGAIN_LATER;
        case EAI_FAIL:
            return ConnectStatus::PERMANENT_FAILURE;
        case EAI_MEMORY:
            return ConnectStatus::OUT_OF_MEMORY;
        case EAI_NODATA:
            return ConnectStatus::NOT_OPEN;
        case EAI_SYSTEM:
            return ConnectStatus::SYSTEM_ERROR;
        case EAI_BADFLAGS:
        case EAI_FAMILY:
        case EAI_SERVICE:
        case EAI_SOCKTYPE:
            // Igonring codes above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_bind_error(const int error_code)
    {
        switch (error_code)
        {
        case EACCES:
            return ConnectStatus::PERMISSION_DENIED;
        case EADDRINUSE:
        case EADDRNOTAVAIL:
            return ConnectStatus::ADDRESS_NOT_AVAILABLE;
        case EBADF:
        case ENOTSOCK:
            return ConnectStatus::NOT_CONNECTED;
        case EINVAL:
            return ConnectStatus::INVALID_ADDRESS;
        // The following errors are specific to UNIX domain (AF_UNIX) sockets:
        case EFAULT:
        case ELOOP:
        case ENAMETOOLONG:
        case ENOENT:
        case ENOMEM:
        case ENOTDIR:
        case EROFS:
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_close_error(const int error_code)
    {
        switch (error_code)
        {
        case EBADF:
            return ConnectStatus::NOT_CONNECTED;
        case EINTR:
            return ConnectStatus::INTERRUPTED_BY_A_SIGNAL;
        case EIO:
            return ConnectStatus::IO_ERROR;
        case ENOSPC:
        case EDQUOT:
            // Ignoring codes above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_connect_error(const int error_code)
    {
        switch (error_code)
        {
        case EACCES:
        case EPERM:
            return ConnectStatus::PERMISSION_DENIED;
        case EADDRINUSE:
            return ConnectStatus::ADDRESS_ALREADY_IN_USE;
        case EADDRNOTAVAIL:
            return ConnectStatus::ADDRESS_NOT_AVAILABLE;
        case EAGAIN:
        case EALREADY:
            return ConnectStatus::TRY_AGAIN_LATER;
        case EAFNOSUPPORT:
        case EBADF:
        case EFAULT:
        case ENOTSOCK:
            return ConnectStatus::INVALID_ADDRESS;
        case ECONNREFUSED:
            return ConnectStatus::CONNECTION_REFUSED;
        case EINPROGRESS:
            return ConnectStatus::CONNECTION_IN_PROGRESS;
        case EINTR:
            return ConnectStatus::INTERRUPTED_BY_A_SIGNAL;
        case EISCONN:
            return ConnectStatus::ALREADY_OPEN;
        case ENETUNREACH:
            return ConnectStatus::NETWORK_UNREACHABLE;
        case ETIMEDOUT:
            return ConnectStatus::TIMED_OUT;
        case EPROTOTYPE:
            // Ignoring codes above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_inet_ntop_error(const int error_code)
    {
        switch (error_code)
        {
        case EAFNOSUPPORT:
        case ENOSPC:
            return ConnectStatus::INVALID_ADDRESS;
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_listen_error(const int error_code)
    {
        switch (error_code)
        {
        case ECONNREFUSED:
            return ConnectStatus::QUEUE_FULL;
        case EADDRINUSE:
            return ConnectStatus::ADDRESS_ALREADY_IN_USE;
        case ENOBUFS:
            return ConnectStatus::OUT_OF_MEMORY;
        case EBADF:
        case ENOTSOCK:
        case EINVAL:
            return ConnectStatus::NOT_CONNECTED;
        case EOPNOTSUPP:
            return ConnectStatus::OPERATION_NOT_SUPPORTED;
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_socket_error(const int error_code)
    {
        switch (error_code)
        {
        case EACCES:
            return ConnectStatus::NO_ACCESS;
        case EMFILE:
        case ENFILE:
            return ConnectStatus::MAXIMUM_CONNECTION_COUNT_REACHED;
        case ENOBUFS:
        case ENOMEM:
            return ConnectStatus::OUT_OF_MEMORY;
        case EPROTONOSUPPORT:
            return ConnectStatus::PROTOCOL_NOT_SUPPORTED_BY_ADDRESS;
        case EAFNOSUPPORT:
        case EINVAL:
            // Igonring codes above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }

    ConnectStatus ErrorHandler::get_socket_option_error(const int error_code)
    {
        switch (error_code)
        {
        case EINVAL:
        case ENOTSOCK:
        case EBADF:
            return ConnectStatus::NOT_CONNECTED;
        case ENOMEM:
        case ENOBUFS:
            return ConnectStatus::OUT_OF_MEMORY;
        case EDOM:
        case EISCONN:
        case ENOPROTOOPT:
            // Igonring codes above
        default:
            return ConnectStatus::UNKNOWN_ERROR;
        }
    }
} // namespace: ramrod::socket
