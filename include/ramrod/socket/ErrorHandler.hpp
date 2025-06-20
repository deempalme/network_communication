#ifndef RAMROD_SOCKET_ERROR_HANDLER_HPP
#define RAMROD_SOCKET_ERROR_HANDLER_HPP

#include "ramrod/socket/Enumerators.hpp" // for ErrorType

#include <array>       // for array
#include <sys/types.h> // for ssize_t

namespace ramrod::socket
{
    class ErrorHandler
    {
    public:
        ErrorHandler() = default;
        virtual ~ErrorHandler() = default;

        /**
         * @brief Get a standard \b errno code from a \b ErrorType enum.
         *
         * @param[in] error_type  ErrorType enum returned by a ramrod::socket function
         *
         * @return A standard errno, some values in enum may not be equivalent to an errno
         *         in such case, the last value in errno when ErrorType was encountered is
         *         returned and it may not match with \p error_type, if the value is zero
         *         then, it means that \p error_type does not have a \b errno
         */
        int get_errno(const ErrorType error_type);

        /**
         * @brief Get the full description of the encountered error.
         *
         * @param[in] error_type  Error code returned by a function
         *
         * @return String with the detailed description of the encountered error
         */
        const char *get_error_detail(const ErrorType error_type);

    protected:
        /**
         * @brief Fill a connection status from \p recv/from() set error.
         *
         * @param[in] error_code       errno set by \p recv/from()
         * @param[in] received_length  Returned value from \p recv/from()
         * @param[out] status          Status that will be filled based on \p error_code,
         *                             ignored if is nullptr
         *
         * @return True if connection should be closed since error means it is
         *         already disconnected
         */
        bool fill_receive_error(const int error_code,
                                const ssize_t received_length,
                                ReceiveStatus *status);
        /**
         * @brief Fill a connection status from \p send/to() set error.
         *
         * @param[in] error_code   errno set by \p send/to()
         * @param[in] sent_length  Returned value from \p send/to()
         * @param[out] status      Status that will be filled based on \p error_code,
         *                         ignored if is nullptr
         *
         * @return True if connection should be closed since error means it is
         *         already disconnected
         */
        bool fill_send_error(const int error_code,
                             const ssize_t sent_length,
                             SendStatus *status);

        /**
         * @brief Get a connection status from \p getaddrinfo() returned error.
         *
         * @param[in] error_code  Error code returned by \p getaddrinfo()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_addr_info_error(const int error_code);

        /**
         * @brief Get a connection status from \p close() errno value.
         *
         * @param[in] error_code  errno value set by \p close()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_close_error(const int error_code);

        /**
         * @brief Get a connection status from \p connect() errno value.
         *
         * @param[in] error_code  errno value set by \p connect()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_connect_error(const int error_code);

        /**
         * @brief Get a connection status from \p inet_ntop() errno value.
         *
         * @param[in] error_code  errno value set by \p inet_ntop()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_inet_ntop_error(const int error_code);

        /**
         * @brief Get a connection status from \p socket() errno value.
         *
         * @param[in] error_code  errno value set by \p socket()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_socket_error(const int error_code);

        /**
         * @brief Get a connection status from \p setsockopt() errno value.
         *
         * @param[in] error_code  errno value set by \p setsockopt()
         *
         * @return An equivalent ConnectStatus value for \p setsockopt
         */
        ConnectStatus get_socket_option_error(const int error_code);

        /**
         * @brief Set the error code for a defined ErrorType.
         *
         * @param[in] error_type  ErrorType enum returned by a ramrod::socket function
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return Same value than \p error_type (passthrough)
         */
        ErrorType set_error_code(const ErrorType error_type, const int error_code);

        /**
         * @brief Set the error code for a standard errno.
         *
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return ErrorType enum equivalent to errno
         */
        ErrorType set_error_code(const int error_code);

    private:
        /**
         * @brief Get ErrorType from a standard socket's returned code.
         *
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return An error enum compatible with ramrod::socket
         */
        ErrorType get_error_type(const int error_code);

        /// @brief Stores the lastest error codes for each ErrorType
        std::array<int, static_cast<size_t>(ErrorType::UNKNOWN_ERROR)> error_codes_{};
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_ERROR_HANDLER_HPP
