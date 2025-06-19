#include "ramrod/socket/Client.hpp"

#include <arpa/inet.h>
#include <cerrno>       // for errno
#include <cstdlib>      // for realloc
#include <cstring>      // for memset
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
#include <sys/socket.h> // for recv, send, MSG_NOSIGNAL, accept
#include <sys/types.h>  // for ssize_t
#include <sys/wait.h>   // for waitpid, WNOHANG
#include <unistd.h>     // for close

namespace
{
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};
    /// @brief Value that indicates an error when receiving or sending data
    static constexpr ssize_t TRANSFER_ERROR{-1l};

    /**
     * @brief Fill server address from an incoming sockaddr.
     *
     * @param[out] server_address  This will be filled with the incoming server address
     * @param[in] socket_type
     * @param[in] in_address       Incoming server address in sockaddr format
     * @param[in] in_length        Length of \p in_address in bytes
     *
     * @return Size of server address in bytes
     */
    std::uint32_t fill_server_address(void *server_address,
                                      const ramrod::socket::SocketType socket_type,
                                      sockaddr *in_address,
                                      const socklen_t in_length)
    {
        if (socket_type == ramrod::socket::SocketType::STREAM)
        {
            static constexpr std::uint32_t EMPTY{};
            // No need to save server address
            return EMPTY;
        }

        const std::size_t address_length{static_cast<std::size_t>(in_length)};
        std::realloc(server_address, address_length);
        std::memcpy(server_address, static_cast<void *>(in_address), address_length);
        return in_length;
    }

    /**
     * @brief Get IPv4 or IPv6 address from \b sockaddr.
     *
     * @param[in] sa     Sockect address structure returned from \b getaddrinfo()
     * @param[out] port  Will set this value from incoming \p sa port
     *
     * @return a void pointer to an IPv4's in_addr or IPv6's in6_addr compatible with \b inet_ntop()
     */
    void *get_in_address(struct sockaddr *sa, std::uint16_t &port)
    {
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (sa->sa_family == AF_INET)
        {
            // IPv4
            ipv4 = reinterpret_cast<struct sockaddr_in *>(sa);
            port = ipv4->sin_port;
            return static_cast<void *>(&ipv4->sin_addr);
        }

        // IPv6
        ipv6 = reinterpret_cast<struct sockaddr_in6 *>(sa);
        port = ipv6->sin6_port;
        return static_cast<void *>(&ipv6->sin6_addr);
    }
} // Unnamed namespace

namespace ramrod::socket
{
    Client::Client()
        : BasicSocket{},
          Conversor{},
          ErrorHandler{},
          server_address_{nullptr},
          server_address_length_{}
    {
    }

    Client::~Client()
    {
        if (server_address_ != nullptr)
            std::free(server_address_);
        disconnect();
    }

    ErrorType Client::connect(const std::string &ip,
                              const std::uint16_t port,
                              const Family ip_family,
                              const SocketType socket_type)
    {
        if (_socket_params.fd != BAD_SOCKET)
        {
            return ErrorType::ALREADY_OPEN;
        }

        static constexpr uint16_t EMPTY_PORT{};
        const bool port_is_empty{port == EMPTY_PORT};
        if (ip.empty() && port_is_empty)
        {
            return ErrorType::IP_AND_PORT_CANNOT_BE_EMPTY;
        }
        const std::string service{port_is_empty ? std::string{} : std::to_string(port)};

        return connect(ip, service, ip_family, socket_type);
    }

    ErrorType Client::connect(const std::string &ip,
                              const std::string &service,
                              const Family ip_family,
                              const SocketType socket_type)
    {
        if (_socket_params.fd != BAD_SOCKET)
        {
            return ErrorType::ALREADY_OPEN;
        }

        if (ip.empty() && service.empty())
        {
            return ErrorType::IP_AND_SERVICE_CANNOT_BE_EMPTY;
        }

        int status{};

        _ip = ip;
        _service = service;
        _family = ip_family;
        _socket_type = socket_type;

        const int family{convert_family(ip_family)};

        struct addrinfo hints{};
        // make sure the struct is empty
        std::memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = family;
        hints.ai_socktype = convert_socket_type(socket_type);
        // fill in my IP for me
        hints.ai_flags = AI_PASSIVE;

        // Making sure that nullptr is passed when string is empty
        const char *ip_pointer{ip.empty() ? nullptr : ip.c_str()};
        const char *service_pointer{ip.empty() ? nullptr : ip.c_str()};
        /// All found socket are stored in this list
        struct addrinfo *results{nullptr};

        /// Status indicating that socket's functions are working properly
        static constexpr int OK{};
        // Get all available devices that can be connected
        if ((status = ::getaddrinfo(ip_pointer, service_pointer, &hints, &results)) != OK)
        {
            if (results != nullptr)
                ::freeaddrinfo(results);

            ConnectStatus connection_status{ConnectStatus::SUCCESS};
            switch (status)
            {
            case EAI_ADDRFAMILY:
                //     The specified network host does not have any network
                //   addresses in the requested address family.
                connection_status = ConnectStatus::;
                break;
            case EAI_AGAIN:
                //     The name server returned a temporary failure indication.
                //   Try again later.
                connection_status = ConnectStatus::;
                break;
            case EAI_BADFLAGS:
                //     hints.ai_flags contains invalid flags; or, hints.ai_flags
                //   included AI_CANONNAME and node was NULL.
                connection_status = ConnectStatus::;
                break;
            case EAI_FAIL:
                // The name server returned a permanent failure indication.
                connection_status = ConnectStatus::;
                break;
            case EAI_FAMILY:
                // The requested address family is not supported.
                connection_status = ConnectStatus::;
                break;
            case EAI_MEMORY:
                // Out of memory.
                connection_status = ConnectStatus::;
                break;
            case EAI_NODATA:
                //     The specified network host exists, but does not have any
                //   network addresses defined.
                connection_status = ConnectStatus::;
                break;
            case EAI_NONAME:
                //     The node or service is not known; or both node and service
                //   are NULL; or AI_NUMERICSERV was specified in hints.ai_flags
                //   and service was not a numeric port-number string.
                connection_status = ConnectStatus::;
                break;
            case EAI_SERVICE:
                //     The requested service is not available for the requested
                //   socket type.  It may be available through another socket
                //   type.  For example, this error could occur if service was
                //   "shell" (a service available only on stream sockets), and
                //   either hints.ai_protocol was IPPROTO_UDP, or
                //   hints.ai_socktype was SOCK_DGRAM; or the error could occur
                //   if service was not NULL, and hints.ai_socktype was SOCK_RAW
                //   (a socket type that does not support the concept of
                //   services).
                connection_status = ConnectStatus::;
                break;
            case EAI_SOCKTYPE:
                //     The requested socket type is not supported.  This could
                //   occur, for example, if hints.ai_socktype and
                //   hints.ai_protocol are inconsistent (e.g., SOCK_DGRAM and
                //   IPPROTO_TCP, respectively).
                connection_status = ConnectStatus::;
                break;
            case EAI_SYSTEM:
                // Other system error; errno is set to indicate the error.
                connection_status = ConnectStatus::;
                break;
            default:
                connection_status = ConnectStatus::UNKNOWN_ERROR;
                break;
            }
            return set_error_code(status);
        }

        /// Pointer to server info
        struct addrinfo *server{nullptr};
        /// Socket's IP string
        char socket_ip_string[INET6_ADDRSTRLEN];
        /// Last registered error (if there is one) used for for-loop function uses
        /// continue rather than return
        int last_error_code{};

        // Loop through all found devices
        for (server = results; server != nullptr; server = server->ai_next)
        {
            // Creating endpoint for communication
            if ((_socket_params.fd = ::socket(server->ai_family,
                                              server->ai_socktype,
                                              server->ai_protocol)) == ERROR)
            {
                last_error_code = errno;
                set_error_code(ErrorType::CREATE_SOCKET_ERROR, errno);

                ConnectStatus connection_status{ConnectStatus::SUCCESS};
                switch (errno)
                {
                case EACCES:
                    //     Permission to create a socket of the specified type and/or
                    //   protocol is denied.
                    connection_status = ConnectStatus::;
                    break;
                case EAFNOSUPPORT:
                    //   The implementation does not support the specified address
                    //   family.
                    connection_status = ConnectStatus::;
                    break;
                case EINVAL:
                    // Unknown protocol, or protocol family not available.
                    // Invalid flags in type.
                    connection_status = ConnectStatus::;
                    break;
                case EMFILE:
                    //     The per-process limit on the number of open file
                    //   descriptors has been reached.
                    connection_status = ConnectStatus::;
                    break;
                case ENFILE:
                    //     The system-wide limit on the total number of open files has
                    //   been reached.
                    connection_status = ConnectStatus::;
                    break;
                case ENOBUFS:
                case ENOMEM:
                    //     Insufficient memory is available.  The socket cannot be
                    //   created until sufficient resources are freed.
                    connection_status = ConnectStatus::;
                    break;
                case EPROTONOSUPPORT:
                    //     The protocol type or the specified protocol is not
                    //   supported within this domain.
                    connection_status = ConnectStatus::;
                    break;
                default:
                    connection_status = ConnectStatus::UNKNOWN_ERROR;
                    break;
                }
                continue;
            }

            // Lose the pesky "Address already in use" error message
            if (::setsockopt(_socket_params.fd,
                             SOL_SOCKET,
                             SO_REUSEADDR,
                             &status,
                             sizeof(int)) == ERROR)
            {
                last_error_code = errno;
                set_error_code(ErrorType::SET_SOCKET_OPTION_ERROR, errno);

                ConnectStatus connection_status{ConnectStatus::SUCCESS};
                switch (errno)
                {
                case EBADF:
                    // The socket argument is not a valid file descriptor.
                    connection_status = ConnectStatus::;
                    break;
                case EDOM:
                    //     The send and receive timeout values are too big to fit into
                    //   the timeout fields in the socket structure.
                    connection_status = ConnectStatus::;
                    break;
                case EINVAL:
                    //     The specified option is invalid at the specified socket
                    //   level or the socket has been shut down.
                    connection_status = ConnectStatus::;
                    break;
                case EISCONN:
                    //     The socket is already connected, and a specified option
                    //   cannot be set while the socket is connected.
                    connection_status = ConnectStatus::;
                    break;
                case ENOPROTOOPT:
                    // The option is not supported by the protocol.
                    connection_status = ConnectStatus::;
                    break;
                case ENOTSOCK:
                    // The socket argument does not refer to a socket.
                    connection_status = ConnectStatus::;
                    break;
                case ENOMEM:
                    //     There was insufficient memory available for the operation
                    //   to complete.
                    connection_status = ConnectStatus::;
                    break;
                case ENOBUFS:
                    //     Insufficient resources are available in the system to
                    //   complete the call.
                    connection_status = ConnectStatus::;
                    break;
                default:
                    connection_status = ConnectStatus::UNKNOWN_ERROR;
                    break;
                }
                ::close(_socket_params.fd);
                continue;
            }

            // convert the IP to a string
            if (::inet_ntop(server->ai_family,
                            get_in_address(server->ai_addr, _socket_params.port),
                            socket_ip_string,
                            sizeof(socket_ip_string)) == nullptr)
            {
                last_error_code = errno;
                set_error_code(ErrorType::IP_CONVERSION_FAILED, errno);

                ConnectStatus connection_status{ConnectStatus::SUCCESS};
                switch (errno)
                {
                case EAFNOSUPPORT:
                    // af was not a valid address family.
                    connection_status = ConnectStatus::;
                    break;
                case ENOSPC:
                    //     The converted address string would exceed the size given by
                    //   size.
                    connection_status = ConnectStatus::;
                    break;
                default:
                    connection_status = ConnectStatus::UNKNOWN_ERROR;
                    break;
                }
                ::close(_socket_params.fd);
                continue;
            }

            // Only TCP allows connections
            if (socket_type == SocketType::STREAM)
            {
                // Connecting to server
                if (::connect(_socket_params.fd, server->ai_addr, server->ai_addrlen) == ERROR)
                {
                    last_error_code = errno;
                    set_error_code(ErrorType::CONNECT_SERVER_ERROR, errno);

                    ConnectStatus connection_status{ConnectStatus::SUCCESS};
                    switch (errno)
                    {
                    case EACCES:
                        //     For UNIX domain sockets, which are identified by pathname:
                        //   Write permission is denied on the socket file, or search
                        //   permission is denied for one of the directories in the path
                        //   prefix.  (See also path_resolution(7).)

                        //     It can also be returned if an SELinux policy denied a
                        //   connection (for example, if there is a policy saying that
                        //   an HTTP proxy can only connect to ports associated with
                        //   HTTP servers, and the proxy tries to connect to a different
                        //   port).
                        connection_status = ConnectStatus::;
                        break;
                    case EPERM:
                        //     The user tried to connect to a broadcast address without
                        //   having the socket broadcast flag enabled or the connection
                        //   request failed because of a local firewall rule.
                        connection_status = ConnectStatus::;
                        break;
                    case EADDRINUSE:
                        // Local address is already in use.
                        connection_status = ConnectStatus::;
                        break;
                    case EADDRNOTAVAIL:
                        //     (Internet domain sockets) The socket referred to by sockfd
                        //   had not previously been bound to an address and, upon
                        //   attempting to bind it to an ephemeral port, it was
                        //   determined that all port numbers in the ephemeral port
                        //   range are currently in use.  See the discussion of
                        //   /proc/sys/net/ipv4/ip_local_port_range in ip(7).
                        connection_status = ConnectStatus::;
                        break;
                    case EAFNOSUPPORT:
                        //     The passed address didn't have the correct address family
                        //   in its sa_family field.
                        connection_status = ConnectStatus::;
                        break;
                    case EAGAIN:
                        //     or nonblocking UNIX domain sockets, the socket is
                        //   nonblocking, and the connection cannot be completed
                        //   immediately.  For other socket families, there are
                        //   insufficient entries in the routing cache.
                        connection_status = ConnectStatus::;
                        break;
                    case EALREADY:
                        //     The socket is nonblocking and a previous connection attempt
                        //   has not yet been completed.
                        connection_status = ConnectStatus::;
                        break;
                    case EBADF:
                        // sockfd is not a valid open file descriptor.
                        connection_status = ConnectStatus::;
                        break;
                    case ECONNREFUSED:
                        //     A connect() on a stream socket found no one listening on
                        //   the remote address.
                        connection_status = ConnectStatus::;
                        break;
                    case EFAULT:
                        //     The socket structure address is outside the user's address
                        //   space.
                        connection_status = ConnectStatus::;
                        break;
                    case EINPROGRESS:
                        //     The socket is nonblocking and the connection cannot be
                        //   completed immediately.  (UNIX domain sockets failed with
                        //   EAGAIN instead.)  It is possible to select(2) or poll(2)
                        //   for completion by selecting the socket for writing.  After
                        //   select(2) indicates writability, use getsockopt(2) to read
                        //   the SO_ERROR option at level SOL_SOCKET to determine
                        //   whether connect() completed successfully (SO_ERROR is zero)
                        //   or unsuccessfully (SO_ERROR is one of the usual error codes
                        //   listed here, explaining the reason for the failure).
                        connection_status = ConnectStatus::;
                        break;
                    case EINTR:
                        //     The system call was interrupted by a signal that was
                        //   caught
                        connection_status = ConnectStatus::;
                        break;
                    case EISCONN:
                        // The socket is already connected.
                        connection_status = ConnectStatus::;
                        break;
                    case ENETUNREACH:
                        //  Network is unreachable.
                        connection_status = ConnectStatus::;
                        break;
                    case ENOTSOCK:
                        // The file descriptor sockfd does not refer to a socket.
                        connection_status = ConnectStatus::;
                        break;
                    case EPROTOTYPE:
                        //     The socket type does not support the requested
                        //   communications protocol.  This error can occur, for
                        //   example, on an attempt to connect a UNIX domain datagram
                        //   socket to a stream socket.
                        connection_status = ConnectStatus::;
                        break;
                    case ETIMEDOUT:
                        //     Timeout while attempting connection.  The server may be too
                        //   busy to accept new connections.  Note that for IP sockets
                        //   the timeout may be very long when syncookies are enabled on
                        //   the server.
                        connection_status = ConnectStatus::;
                        break;
                    default:
                        connection_status = ConnectStatus::UNKNOWN_ERROR;
                        break;
                    }
                    ::close(_socket_params.fd);
                    continue;
                }
            }

            _socket_params.ip = socket_ip_string;
            _socket_params.family = convert_family(server->ai_family);
            _socket_params.type = convert_socket_type(server->ai_socktype);

            server_address_length_ = fill_server_address(server_address_,
                                                         _socket_params.type,
                                                         server->ai_addr,
                                                         server->ai_addrlen);

            break;
        }

        // Free result's memory
        if (results != nullptr)
            ::freeaddrinfo(results);

        if (server == nullptr)
        {
            ::close(_socket_params.fd);
            _socket_params = {};
            _socket_params.fd = BAD_SOCKET;
            return set_error_code(ErrorType::NO_SERVER_AVAILABLE, last_error_code);
        }

        if (_socket_params.fd == BAD_SOCKET)
        {
            return set_error_code(ErrorType::NO_SERVER_AVAILABLE, last_error_code);
        }

        return ErrorType::SUCCESS;
    }

    ConnectStatus Client::disconnect()
    {
        ConnectStatus status{ConnectStatus::SUCCESS};

        if (_socket_params.fd != BAD_SOCKET)
        {
            if (::close(_socket_params.fd) == ERROR)
            {
                switch (errno)
                {
                case EBADF:
                    status = ConnectStatus::NOT_CONNECTED;
                    break;
                case EINTR:
                    status = ConnectStatus::INTERRUPTED_BY_A_SIGNAL;
                    break;
                case EIO:
                    status = ConnectStatus::IO_ERROR;
                    break;
                case ENOSPC:
                case EDQUOT:
                    // Ignoring those errors
                    break;
                default:
                    status = ConnectStatus::UNKNOWN_ERROR;
                    break;
                }
            }
            _socket_params = {};
            _socket_params.fd = BAD_SOCKET;
        }

        return status;
    }

    bool Client::is_connected()
    {
        return _socket_params.fd != BAD_SOCKET;
    }

    ssize_t Client::receive(void *buffer, const std::size_t size, ReceiveStatus *status)
    {
        if (_socket_params.fd == BAD_SOCKET)
        {
            // Not connected and hence early exit
            if (status != nullptr)
                *status = ReceiveStatus::NOT_CONNECTED;
            return TRANSFER_ERROR;
        }

        ssize_t total_received{};

        /// No receive flags
        static constexpr int NO_FLAGS{};

        if (_socket_params.type == SocketType::STREAM)
        {
            // Receiving over TCP
            total_received == ::recv(_socket_params.fd, buffer, size, NO_FLAGS);
        }
        else
        {
            // Receiving over UDP
            total_received == ::recvfrom(_socket_params.fd,
                                         buffer,
                                         size,
                                         NO_FLAGS);
        }

        if (status != nullptr)
        {
            *status = ReceiveStatus::SUCCESS;
            if (total_received == TRANSFER_ERROR)
            {
                switch (errno)
                {
                case EAGAIN:
                    *status = ReceiveStatus::TRY_RECEIVE_AGAIN;
                    break;
                case EBADF:
                case ECONNREFUSED:
                case ENOTCONN:
                case ENOTSOCK:
                    *status = ReceiveStatus::NOT_CONNECTED;
                    disconnect();
                    break;
                case EFAULT:
                    *status = ReceiveStatus::BUFFER_POINTER_FAULT;
                    break;
                case EINTR:
                    *status = ReceiveStatus::INTERRUPTED_BY_A_SIGNAL;
                    break;
                case EINVAL:
                    *status = ReceiveStatus::INVALID_ARGUMENT;
                    break;
                case ENOMEM:
                    *status = ReceiveStatus::NO_MEMORY;
                    break;
                default:
                    *status = ReceiveStatus::UNKNOWN_ERROR;
                    break;
                }
            }
        }

        return total_received;
    }

    ConnectStatus Client::reconnect()
    {
        disconnect();

        if (!is_initialized())
        {
            return ConnectStatus::CONNECTION_HAS_NOT_BEEN_CALLED_YET;
        }
        return connect(_ip, _service, _family, _socket_type);
    }

    ssize_t Client::send(const void *buffer, const std::size_t size, SendStatus *status)
    {
        if (_socket_params.fd == BAD_SOCKET)
        {
            // Not connected and hence early exit
            if (status != nullptr)
                *status = SendStatus::NOT_CONNECTED;
            return TRANSFER_ERROR;
        }

        ssize_t total_sent{};

        if (_socket_params.type == SocketType::STREAM)
        {
            // Sending over TCP
            total_sent = ::send(_socket_params.fd, buffer, size, MSG_NOSIGNAL);
        }
        else
        {
            // Sending over UDP
            total_sent = ::sendto(_socket_params.fd,
                                  buffer,
                                  size,
                                  MSG_NOSIGNAL,
                                  static_cast<sockaddr *>(server_address_),
                                  static_cast<socklen_t>(server_address_length_));
        }

        if (status != nullptr)
        {
            *status = SendStatus::SUCCESS;
            if (total_sent == TRANSFER_ERROR)
            {
                switch (errno)
                {
                case EACCES:
                    *status = SendStatus::NO_ACCESS;
                    break;
                case EAGAIN:
                    *status = SendStatus::TRY_SEND_AGAIN;
                    break;
                case EALREADY:
                case EDESTADDRREQ:
                case EISCONN:
                case EOPNOTSUPP:
                    // Ignoring these error codes
                    break;
                case EBADF:
                case EFAULT:
                case ENOTCONN:
                case ENOTSOCK:
                case EPIPE:
                    *status = SendStatus::NOT_CONNECTED;
                    disconnect();
                    break;
                case ECONNRESET:
                    *status = SendStatus::CONNECTION_RESET_BY_PEER;
                    break;
                case EINTR:
                    *status = SendStatus::INTERRUPTED_BY_A_SIGNAL;
                    break;
                case EINVAL:
                    *status = SendStatus::INVALID_ARGUMENT;
                    break;
                case EMSGSIZE:
                    *status = SendStatus::BAD_MESSAGE_SIZE;
                    break;
                case ENOBUFS:
                    *status = SendStatus::OUTPUT_QUEUE_FULL;
                    break;
                case ENOMEM:
                    *status = SendStatus::NO_MEMORY_AVAILABLE;
                    break;
                default:
                    *status = SendStatus::UNKNOWN_ERROR;
                    break;
                }
            }
        }

        return total_sent;
    }
} // namespace: ramrod::socket