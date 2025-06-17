#ifndef RAMROD_SOCKET_SERVER_HPP
#define RAMROD_SOCKET_SERVER_HPP

#include <cstdint>     // for uint32_t, uint16_t
#include <string>      // for string
#include <sys/types.h> // for ssize_t

#include "ramrod/socket/Conversor.hpp"   // for Conversor
#include "ramrod/socket/Enumerators.hpp" // for Protocol, SocketType

struct addrinfo;

namespace ramrod::socket
{
    class Server : public Conversor
    {
    public:
        Server();
        virtual ~Server();

        /**
         * @brief Connect to client.
         *
         * @param[in] ip           IP address to connect
         * @param[in] port         Port number to where the connection will be made
         * @param[in] ip_protocol  Defines the IP version to use
         * @param[in] socket_type  Defines the type of connection
         *
         * @return False if connection failed or it is already connected
         */
        bool connect(const std::string &ip,
                     const std::uint16_t port,
                     const Protocol ip_protocol = Protocol::IPV4,
                     const SocketType socket_type = SocketType::STREAM);

        /**
         * @brief Disconnect this server from the current connected client.
         *
         * @return False if the connection cannot be closed
         */
        bool disconnect();

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
        Protocol ip_protocol();

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
         * @return String with the detailes description of the last encountered error
         */
        const std::string_view& last_error_detail();

        /**
         * @brief Get current port.
         *
         * @return Port's value
         */
        std::uint16_t port();

        /**
         * @brief Receive data from a socket stream.
         *
         * @param buffer Is a pointer to the data you want to receive
         * @param size   Is the number of bytes you want to receive
         * @param flags  Allows you to specify more information about how the data is to be received.
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
         * @brief Reconnect using same parameters.
         *
         * This will disconnect any previous connection (if exist) and try to connect again
         * to the network's device selected in the function `connect()`, and, as in `connect()`
         * it will also be performed in a different thread.
         *
         * @param concurrent Indicates if the reconnection should be made in a different thread,
         *                   in this way the main thread should not await for the server to
         *                   connect with us
         *
         * @return `false` if there is no IP or port selected, it will return `true` if
         *          there is an open pending connection, or already waiting for connection
         */
        bool reconnect();

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
        std::string ip_;
        Protocol ip_protocol_;
        std::uint16_t port_;
        SocketType socket_type_;

        int socket_fd_;
        int connected_fd_;

        bool connected_;
        ErrorType last_error_;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_SERVER_HPP
