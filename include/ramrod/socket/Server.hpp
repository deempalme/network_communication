#ifndef RAMROD_SOCKET_SERVER_HPP
#define RAMROD_SOCKET_SERVER_HPP

#include "ramrod/socket/BasicSocket.hpp"  // for BasicSocket
#include "ramrod/socket/ChildClient.hpp"  // for ChildClient
#include "ramrod/socket/Conversor.hpp"    // for Conversor
#include "ramrod/socket/Enumerators.hpp"  // for ErrorType, Family, SocketType
#include "ramrod/socket/ErrorHandler.hpp" // for ErrorHandler

#include <cstdint> // for uint16_t
#include <memory>  // for shared_ptr
#include <string>  // for string

namespace ramrod::socket
{
    class Server : public BasicSocket, public Conversor, public ErrorHandler
    {
    public:
        /**
         * @brief Initialize inheritance.
         */
        Server();

        /**
         * @brief Closes server socket and all alive connections.
         */
        virtual ~Server();

        /**
         * @brief Open a server socket that allows connection to client.
         *
         * This must be called at least once before any call to \p listen() is made,
         * it creates a server that will allow client connections, you can limit who
         * connects by using \p port, \p ip_family, and \p socket_type
         *
         * If \p port is zero, then the port number of the returned socket addresses
         * will be left uninitialized.
         *
         * \p port numbers above 0 and below 1024 are reserved (superusers may use them).
         *
         * If \p ip_family is set to UNSPECIFIED, then either IPv4 or IPv6 could be used
         * based on availability.
         *
         * @param[in] port         Port number to where the connection will be made
         * @param[in] ip_family    Defines the IP version to use
         * @param[in] socket_type  Defines the type of connection
         * @param[in] queue        Defines the max number of pending connections on queue
         *                         before new are rejected, works only on \p SocketType::STREAM
         *
         * @return SUCCESS if there are no errors, use \p get_error_detail() to see
         *         the full description of the error.
         */
        ConnectStatus open(const std::uint16_t port,
                           const Family ip_family = Family::IPV4,
                           const SocketType socket_type = SocketType::STREAM,
                           const std::uint32_t queue = 20u);

        /**
         * @brief Same as open() but it is possible to also use a string instead of just
         *        a port number.
         *
         * It is possible to define a \p service with a string suhclike "http" or use a port
         * number instead "1234".
         *
         * Everything else should behave the same as \b open() funtion above.
         *
         * @param[in] service      A string containing a service name or a port number
         * @param[in] ip_family    Defines the IP version to use
         * @param[in] socket_type  Defines the type of connection
         * @param[in] queue        Defines the max number of pending connections on queue
         *                         before new are rejected, works only on \p SocketType::STREAM
         *
         * @return SUCCESS if there are no errors, use \p get_error_detail() to see
         *         the full description of the error.
         */
        ConnectStatus open(const std::string &service,
                           const Family ip_family = Family::IPV4,
                           const SocketType socket_type = SocketType::STREAM,
                           const std::uint32_t queue = 20u);

        /**
         * @brief Close this server to all clients.
         *
         * All client's connection will also be closed.
         *
         * @return  It will return error when server cannot be closed
         */
        ConnectStatus close();

        /**
         * @brief Check if server is still open.
         *
         * @return True when server is still open for communications with clients
         */
        bool is_open();

        /**
         * @brief Accept a single pending connection from a client.
         *
         * TODO: fill
         *
         * @param[out] status  If not nullptr then, this will be set with the status of
         *                     this function, it should return \p SUCCESS if everything
         *                     goes well, but if returned pointer is nullptr a different
         *                     status will be set
         *
         * @return TODO: fill
         */
        std::shared_ptr<ChildClient> accept(ConnectStatus *status = nullptr);

        /**
         * @brief Recreate server using same parameters.
         *
         * This will destroy any previos server and disconnect any previous client (if exist)
         * and try to create server again using the same parameters than utilized in last
         * call to \p open(), you must look for clients once more.
         *
         * @return  Error if server has never been open before, or an error value similar
         *          than the returned from \p open()
         */
        ConnectStatus reopen();

    private:
        /// @brief Max number of pending connections on queue before new are rejected
        int max_queue_count_;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_SERVER_HPP
