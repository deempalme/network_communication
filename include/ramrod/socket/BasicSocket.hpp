#ifndef RAMROD_SOCKET_BASIC_SOCKET_HPP
#define RAMROD_SOCKET_BASIC_SOCKET_HPP

#include "ramrod/socket/Enumerators.hpp" // for ErrorType, Family, SocketType

#include <array>   // for array
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
         * @brief Get hostname from current device.
         *
         * @return String containing current device's hostname, it may be empty if there
         *         was an error while getting hostname
         */
        const std::string &hostname();

        /**
         * @brief Get current device's IP address.
         *
         * @param[in] family  IP family version, you can choose \p Family::UNSPECIFIED
         *                    to return the IP that matches the current family
         *
         * @return String containing current device's IP address, if \p family is
         *         equal to \p Family::UNSPECIFIED then IP family should be equal to
         *         the current used family, or IPv4 by default if none is in use. It
         *         will be empty if getting hostname failed
         */
        const std::string &ip(const Family family = Family::UNSPECIFIED;

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

        /**
         * @brief Reap dead children processes.
         *
         * Removes all children processes that have already finished but still lingering
         * in mermoy, often called zombies.
         *
         * It will perform a wait to allow the system to release the resources associated with
         * the child. If wait is not performed, then the terminated child remains in a zombie
         * state.
         *
         * @param[in] stop  If true then, it will destroy signal's action that reap child
         *                  processes
         *
         * @return False if failed
         */
        static bool read_dead_processes(const bool stop = false);

        /**
         * @brief Shutdown receive, send or both communications from a connection.
         *
         * @param[in] type  Indicates which action should be shutdown: RECEIVE, SEND, or ALL
         *
         * @return Status of the shutdown
         */
        ConnectStatus shutdown(const ShutdownType type = ShutdownType::RECEIVE);

        /**
         * @brief Get current socket type.
         *
         * @return Current socket type
         */
        SocketType socket_type();

    protected:
        /**
         * @brief Check if two sockaddr are equal.
         *
         * Only IPv4 and IPv6 are supported, if there is a not supported family then this
         * will return false
         *
         * @param[in] a  First address to compare
         * @param[in] b  Second address that will be compared
         *
         * @return True if bot addresses are the same, false otherwise
         */
        static bool are_addresses_equal(const void *a, const void *b);

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

        std::string get_ip_address(ConnectStatus *status = nullptr);

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

        /// @brief Current device's hostname
        std::string _device_hostname;
        /// @brief Current device's IPv4 address
        std::string _device_ip4;
        /// @brief Current device's IPv6 address
        std::string _device_ip6;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_BASIC_SOCKET_HPP
