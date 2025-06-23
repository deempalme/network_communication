#ifndef RAMROD_SOCKET_ERROR_HANDLER_HPP
#define RAMROD_SOCKET_ERROR_HANDLER_HPP

#include "ramrod/socket/Enumerators.hpp" // for ConnectStatus, ReceiveStatus, SendStatus

#include <sys/types.h> // for ssize_t

namespace ramrod::socket
{
    class ErrorHandler
    {
    public:
        ErrorHandler() = default;
        virtual ~ErrorHandler() = default;

        /**
         * @brief Get the full description of encountered \p ConnectStatus
         *
         * @param[in] status  \p ConnectStatus returned by a function
         *
         * @return String with the detailed description of the encountered \p ConnectStatus
         */
        const char *get_status_detail(const ConnectStatus status);

        /**
         * @brief Get the full description of encountered \p ReceiveStatus
         *
         * @param[in] status  \p ReceiveStatus returned by a function
         *
         * @return String with the detailed description of the encountered \p ReceiveStatus
         */
        const char *get_status_detail(const ReceiveStatus status);

        /**
         * @brief Get the full description of encountered \p SendStatus
         *
         * @param[in] status  \p SendStatus returned by a function
         *
         * @return String with the detailed description of the encountered \p SendStatus
         */
        const char *get_status_detail(const SendStatus status);

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
         * @brief Get a connection status from \p accept() errno value.
         *
         * @param[in] error_code  errno value set by \p accept()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_accept_error(const int error_code);

        /**
         * @brief Get a connection status from \p getaddrinfo() returned error.
         *
         * @param[in] error_code  Error code returned by \p getaddrinfo()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_addr_info_error(const int error_code);

        /**
         * @brief Get a connection status from \p bind() errno value.
         *
         * @param[in] error_code  errno value set by \p bind()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_bind_error(const int error_code);

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
         * @brief Get a connection status from \p listen() errno value.
         *
         * @param[in] error_code  errno value set by \p listen()
         *
         * @return An equivalent ConnectStatus value for \p error_code
         */
        ConnectStatus get_listen_error(const int error_code);

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
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_ERROR_HANDLER_HPP
