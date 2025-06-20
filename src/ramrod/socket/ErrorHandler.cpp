#include "ramrod/socket/ErrorHandler.hpp"

#include <cerrno>  // for errno
#include <cstdint> // for size_t
#include <cstring> // for stderror
#include <netdb.h> // for EAI_... codes

namespace
{
    /// @brief Value that indicates an error when receiving or sending data
    static constexpr ssize_t TRANSFER_ERROR{-1l};
} // Unnamed namespace

namespace ramrod::socket
{
    /// @brief Id for first element in array (equivalent to ErrorType::SUCCESS)
    static constexpr size_t FIRST_ELEMENT{1ul};

    int ErrorHandler::get_errno(const ErrorType error_type)
    {
        /// id should not include the first enum (ErrorType::SUCCESS)
        const size_t id{static_cast<size_t>(error_type) - FIRST_ELEMENT};
        return error_codes_.at(id);
    }

    const char *ErrorHandler::get_error_detail(const ErrorType error_type)
    {
        switch (error_type)
        {
        case ErrorType::SUCCESS:
            static constexpr char SUCCESS_MSG[]{"No error encountered"};
            return SUCCESS_MSG;
        case ErrorType::ALREADY_OPEN:
            static constexpr char ALREADY_OPEN_MSG[]{"There is already an open server socket"};
            return ALREADY_OPEN_MSG;
        case ErrorType::IP_AND_PORT_CANNOT_BE_EMPTY:
            static constexpr char IP_AND_PORT_CANNOT_BE_EMPTY_MSG[]{
                "IP address and port cannot be empty at the same time"};
            return IP_AND_PORT_CANNOT_BE_EMPTY_MSG;
        case ErrorType::IP_AND_SERVICE_CANNOT_BE_EMPTY:
            static constexpr char IP_AND_SERVICE_CANNOT_BE_EMPTY_MSG[]{
                "IP address and service cannot be empty at the same time"};
            return IP_AND_SERVICE_CANNOT_BE_EMPTY_MSG;
        case ErrorType::OPEN_HAS_NOT_BEEN_CALLED_YET:
            static constexpr char OPEN_HAS_NOT_BEEN_CALLED_YET_MSG[]{
                "Cannot call reopen() if open() has not been called at least once"};
            return OPEN_HAS_NOT_BEEN_CALLED_YET_MSG;
        case ErrorType::PORT_CANNOT_BE_EMPTY:
            static constexpr char PORT_CANNOT_BE_EMPTY_MSG[]{"Port cannot be zero/empty"};
            return PORT_CANNOT_BE_EMPTY_MSG;
        case ErrorType::SERVICE_CANNOT_BE_EMPTY:
            static constexpr char SERVICE_CANNOT_BE_EMPTY_MSG[]{"Service string cannot be empty"};
            return SERVICE_CANNOT_BE_EMPTY_MSG;
        case ErrorType::ADDRESS_INFO_BAD_FLAGS:
        case ErrorType::ADDRESS_INFO_FAMILY_NOT_SUPPORTED:
        case ErrorType::ADDRESS_INFO_NO_ADDRESS_DEFINED:
        case ErrorType::ADDRESS_INFO_NO_NAME:
        case ErrorType::ADDRESS_INFO_OUT_OF_MEMORY:
        case ErrorType::ADDRESS_INFO_PERMANENT_FAILURE:
        case ErrorType::ADDRESS_INFO_SERVICE_NOT_AVAILABLE:
        case ErrorType::ADDRESS_INFO_SOCKET_TYPE_NOT_SUPPORTED:
        case ErrorType::ADDRESS_INFO_TRY_AGAIN_LATER:
        case ErrorType::ADDRESS_INFO_UNKNOWN_ADDRESS_FAMILY:
            return ::gai_strerror(get_errno(error_type));
        case ErrorType::ADDRESS_INFO_SYSTEM_ERROR:
        case ErrorType::BIND_SOCKET_ERROR:
        case ErrorType::CLOSE_ERROR:
        case ErrorType::CONNECT_SERVER_ERROR:
        case ErrorType::CREATE_SOCKET_ERROR:
        case ErrorType::DEAD_PROCESSES_REAPING_CONNECTION_FAILED:
        case ErrorType::IP_CONVERSION_FAILED:
        case ErrorType::NO_SERVER_AVAILABLE:
        case ErrorType::NO_SOCKET_AVAILABLE:
        case ErrorType::SET_SOCKET_OPTION_ERROR:
        case ErrorType::SYSTEM_ERROR:
            return std::strerror(get_errno(error_type));
        default:
        case ErrorType::UNKNOWN_ERROR:
            static constexpr char UNKNOWN_ERROR_MSG[]{"Unknown error"};
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
                final_status = ReceiveStatus::NO_MEMORY;
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
                final_status = SendStatus::NO_MEMORY_AVAILABLE;
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

    ErrorType ErrorHandler::set_error_code(const ErrorType error_type, const int error_code)
    {
        if (error_type == ErrorType::SUCCESS)
        {
            // There is no errno for a successful operation
            return error_type;
        }
        /// id should not include the first enum (ErrorType::SUCCESS)
        const size_t id{static_cast<size_t>(error_type) - FIRST_ELEMENT};
        error_codes_.at(id) = error_code;
        return error_type;
    }

    ErrorType ErrorHandler::set_error_code(const int error_code)
    {
        const ErrorType error_type{get_error_type(error_code)};

        /// id should not include the first enum (ErrorType::SUCCESS)
        const size_t id{static_cast<size_t>(error_type) - FIRST_ELEMENT};
        error_codes_.at(id) = error_code;
        return error_type;
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    ErrorType ErrorHandler::get_error_type(const int error_code)
    {
        using namespace ramrod::socket;

        switch (error_code)
        {
        case EAI_ADDRFAMILY:
            return ErrorType::ADDRESS_INFO_UNKNOWN_ADDRESS_FAMILY;
        case EAI_AGAIN:
            return ErrorType::ADDRESS_INFO_TRY_AGAIN_LATER;
        case EAI_BADFLAGS:
            return ErrorType::ADDRESS_INFO_BAD_FLAGS;
        case EAI_FAIL:
            return ErrorType::ADDRESS_INFO_PERMANENT_FAILURE;
        case EAI_FAMILY:
            return ErrorType::ADDRESS_INFO_FAMILY_NOT_SUPPORTED;
        case EAI_MEMORY:
            return ErrorType::ADDRESS_INFO_OUT_OF_MEMORY;
        case EAI_NODATA:
            return ErrorType::ADDRESS_INFO_NO_ADDRESS_DEFINED;
        case EAI_NONAME:
            return ErrorType::ADDRESS_INFO_NO_NAME;
        case EAI_SERVICE:
            return ErrorType::ADDRESS_INFO_SERVICE_NOT_AVAILABLE;
        case EAI_SOCKTYPE:
            return ErrorType::ADDRESS_INFO_SOCKET_TYPE_NOT_SUPPORTED;
        case EAI_SYSTEM:
            return ErrorType::ADDRESS_INFO_SYSTEM_ERROR;
        default:
            return ErrorType::SYSTEM_ERROR;
        }
    }
} // namespace: ramrod::socket
