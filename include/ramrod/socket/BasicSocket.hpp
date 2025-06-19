#ifndef RAMROD_SOCKET_BASIC_SOCKET_HPP
#define RAMROD_SOCKET_BASIC_SOCKET_HPP

#include "ramrod/socket/Enumerators.hpp" // for ErrorType, Family, SocketType

#include <cstdint> // for uint16_t
#include <string>  // for string

namespace ramrod::socket
{
    class BasicSocket
    {
    public:
        BasicSocket();
        virtual ~BasicSocket() = default;

        /**
         * @brief Get current IP address.
         *
         * @return String containing the IP address
         */
        const std::string &ip();

        /**
         * @brief Get current IP protocol version.
         *
         * @return IP protocol version
         */
        Family ip_family();

        /**
         * @brief Get current port.
         *
         * @return Port's value
         */
        std::uint16_t port();

        // TODO: fill
        ConnectStatus shutdown(const ShutdownType type = ShutdownType::RECEIVE);

        /**
         * @brief Get current socket type.
         *
         * @return Current socket type
         */
        SocketType socket_type();

    protected:
        /**
         * @brief Convert socket::Family into standard family.
         *
         * @param[in] family  IP Family version taken from socket::Family
         *
         * @return Standard IP family version
         */
        int convert_family(const Family family);

        /**
         * @brief Convert standard family into socket::Family.
         *
         * @param[in] family  Standard IP Family version taken from socket
         *
         * @return socket::Family's IP family version
         */
        Family convert_family(const int family);

        /**
         * @brief Convert socket::SocketType into standard socket type.
         *
         * @param[in] type  Socket type
         *
         * @return Standard socket type
         */
        int convert_socket_type(const SocketType type);

        /**
         * @brief Convert standard socket type into socket::SocketType.
         *
         * @param[in] type  Standard socket type taken from socket
         *
         * @return socket::SocketType's socket type
         */
        SocketType convert_socket_type(const int type);

        /**
         * @brief Check if socket parameters has been initialized.
         *
         * @return False if parameters have not been initialized
         */
        bool is_initialized();

        /// @brief IP address where socket should be connected to
        std::string _ip;
        /// @brief Family that socket should have
        Family _family;
        /// @brief Service where socket should be listening
        std::string _service;
        /// @brief Type that socket should have
        SocketType _socket_type;

        struct SocketInfo
        {
            /// @brief Socket's file descriptor
            int fd{};
            /// @brief Socket's IP family version
            Family family{};
            /// @brief Socket's real port
            std::uint16_t port{};
            /// @brief Socket's type
            SocketType type{};
            /// @brief Socket's real IP address
            std::string ip{};
        };
        /// @brief Parameters of server socket
        SocketInfo _socket_params;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_BASIC_SOCKET_HPP
