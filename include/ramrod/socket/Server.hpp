#ifndef RAMROD_SOCKET_SERVER_HPP
#define RAMROD_SOCKET_SERVER_HPP

#include <cstdint>     // for uint32_t, uint16_t
#include <string>      // for string
#include <string_view> // for string_view
#include <sys/types.h> // for ssize_t

#include "ramrod/socket/Conversor.hpp"   // for Conversor
#include "ramrod/socket/Enumerators.hpp" // for ErrorType, Family, SocketType

struct addrinfo;

namespace ramrod::socket
{
    class Server : public Conversor
    {
    public:
        Server();
        virtual ~Server();

        /**
         * @brief Create a server socket that allows connection to client.
         *
         * This must be called at least once before any call to \p connect() is made,
         * it creates a server that will allow client connections, you can limit who
         * connects by using \p ip, \p port, \p ip_family, and \p socket_type
         *
         * If \p ip is empty, then the network address will be set to the loopback
         * interface address; this is used by applications that intend to communicate
         * with peers running on the same host.
         *
         * If \p port is zero, then the port number of the returned socket addresses
         * will be left uninitialized.
         *
         * \p port numbers above 0 and below 1024 are reserved (superusers may use them).
         *
         * Either \p ip or \p port may be empty, but not both.
         *
         * If \p ip_family is set to UNSPECIFIED, then either IPv4 or IPv6 could be used
         * based on availability.
         *
         * @param[in] ip           IP address to connect
         * @param[in] port         Port number to where the connection will be made
         * @param[in] ip_family    Defines the IP version to use
         * @param[in] socket_type  Defines the type of connection
         *
         * @return False if connection failed or it is already connected
         */
        bool create(const std::string &ip,
                     const std::uint16_t port,
                     const Family ip_family = Family::IPV4,
                     const SocketType socket_type = SocketType::STREAM);

        /**
         * @brief Destroy this server socket from the current connected client.
         *
         * All client's connection will also be closed.
         *
         * @return False if the server cannot be destroyed
         */
        bool destroy();

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
         * @brief Indicate if there is connection with client.
         *
         * @return True if there is an open connection
         */
        bool is_connected();

        /**
         * @brief Get last encountered error.
         *
         * If there are two or more encountered errors, then this will only return
         * the last one
         *
         * @return Last encountered error
         */
        ErrorType last_error();

        /**
         * @brief Get the full description of the last encountered error.
         *
         * @return String with the detailed description of the last encountered error
         */
        const char *last_error_detail();

        /**
         * @brief Get current port.
         *
         * @return Port's value
         */
        std::uint16_t port();

        /**
         * @brief Receive data from a socket stream.
         *
         * @param[out] buffer Output buffer where received data will be saved
         * @param[in] size    Is the number of bytes you want to receive
         * @param[in] flags   Allows you to specify more information about how the data is to be received.
         *          MSG_OOB      Receive as “out of band” data. This is how to get data that has
         *                       been sent to you with the `MSG_OOB` flag in `send()`. As the
         *                       receiving side, you will have had signal `SIGURG` raised telling
         *                       you there is urgent data. In your handler for that signal, you
         *                       could call `receive()` with this `MSG_OOB` flag.
         *          MSG_PEEK     If you want to call `receive()` “just for pretend”, you can call
         *                       it with this flag. This will tell you what’s waiting in the
         *                       buffer for when you call `receive()` “for real” (i.e. without
         *                       the `MSG_PEEK` flag. It’s like a sneak preview into the next
         *                       `receive()` call.
         *          MSG_WAITALL  Tell `receive()` to not return until all the data you specified
         *                       in the len parameter. It will ignore your wishes in extreme
         *                       circumstances, however, like if a signal interrupts the call
         *                       or if some error occurs or if the remote side closes the
         *                       connection, etc. Don’t be mad with it.
         *
         * @return The number of bytes actually received, or 0 when the server is disconnected or
         *         size=0, or -1 on error (and `errno` will be set accordingly).
         */
        ssize_t receive(void *buffer, const std::size_t size, const int flags = 0);

        /**
         * @brief Recreate server using same parameters.
         *
         * This will destroy any previos server and disconnect any previous client (if exist)
         * and try to create server again, you must look for clients once more.
         *
         * @return  False if there \p create() has not been called at least once, or true
         *          if server was recreated successfully
         */
        bool recreate();

        /**
         * @brief Send data to a socket stream.
         *
         * @param buffer Is a pointer to the data you want to send
         * @param size   Is the number of bytes you want to send
         * @param flags  Allows you to specify more information about how the data is to be sent.
         *          MSG_OOB       Send as “out of band” data. TCP supports this, and it’s a way to
         *                        tell the receiving system that this data has a higher priority
         *                        than the normal data. The receiver will receive the signal SIGURG
         *                        and it can then receive this data without first receiving all
         *                        the rest of the normal data in the queue.
         *          MSG_DONTROUTE Don’t send this data over a router, just keep it local.
         *          MSG_DONTWAIT  If `send()` would block because outbound traffic is clogged, have
         *                        it return `EAGAIN`. This is like a “enable non-blocking just for
         *                        this send.”
         *          MSG_NOSIGNAL  If you `send()` to a remote host which is no longer
         *                        `receive()`ing, you’ll typically get the signal `SIGPIPE`.
         *                        Adding this flag prevents that signal from being raised.
         *
         * @return The number of bytes actually received, or 0 when the server is disconnected
         *         or if size=0, or if is UDP and you have not yet received a packet to obtain
         *         client address information, or -1 on error (and `errno` will be set accordingly).
         */
        ssize_t send(const void *buffer, const std::size_t size, const int flags = MSG_NOSIGNAL);

        /**
         * @brief Get current socket type.
         *
         * @return Current socket type
         */
        SocketType socket_type();

    private:
        /// @brief IP where socket should be listening
        std::string target_ip_;
        /// @brief Family that socket should have
        Family target_family_;
        /// @brief Port where socket should be listening
        std::uint16_t target_port_;
        /// @brief Type that socket should have
        SocketType target_socket_type_;

        struct SocketInfo
        {
            /// @brief Socket's file descriptor
            int fd{};
            /// @brief Socket's IP family version
            Family family{};
            /// @brief Socket's port
            std::uint16_t port{};
            /// @brief Socket's type
            SocketType type{};
            /// @brief Socket's IP address
            std::string ip{};
        };
        /// @brief Parameters of active socket (listening)
        SocketInfo active_socket_;

        ErrorType last_error_;
        int last_error_code_;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_SERVER_HPP
