#include "ramrod/socket/ErrorHandler.hpp"

#include <cerrno>  // for errno
#include <cstdint> // for size_t
#include <cstring> // for stderror
#include <netdb.h> // for EAI_... codes

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
