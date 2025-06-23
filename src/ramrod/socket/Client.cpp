#include "ramrod/socket/Client.hpp"

#include <arpa/inet.h>  // for inet_ntop
#include <cerrno>       // for errno
#include <cstring>      // for memset, size_t
#include <netdb.h>      // for addrinfo, freeaddrinfo, getaddrinfo, AI_PASSIVE
#include <netinet/in.h> // for sockaddr_in, sockaddr_in6, INET6_ADDRSTRLEN
#include <sys/socket.h> // for connect, recv, send, setsockopt, socket, AF_...
#include <sys/types.h>  // for ssize_t
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
          ErrorHandler{}
    {
    }

    Client::~Client()
    {
        disconnect();
    }

    ConnectStatus Client::connect(const std::string &ip,
                                  const std::uint16_t port,
                                  const Family ip_family)
    {
        if (_socket_params.fd != BAD_SOCKET)
            return ConnectStatus::ALREADY_OPEN;

        static constexpr uint16_t EMPTY_PORT{};
        const bool port_is_empty{port == EMPTY_PORT};
        if (ip.empty() && port_is_empty)
            return ConnectStatus::IP_AND_PORT_CANNOT_BE_EMPTY;

        const std::string service{port_is_empty ? std::string{} : std::to_string(port)};

        return connect(ip, service, ip_family);
    }

    ConnectStatus Client::connect(const std::string &ip,
                                  const std::string &service,
                                  const Family ip_family)
    {
        if (_socket_params.fd != BAD_SOCKET)
            return ConnectStatus::ALREADY_OPEN;

        if (ip.empty() && service.empty())
            return ConnectStatus::IP_AND_SERVICE_CANNOT_BE_EMPTY;

        int status{};

        _ip = ip;
        _service = service;
        _family = ip_family;

        const int family{convert_family(ip_family)};

        struct addrinfo hints{};
        // make sure the struct is empty
        std::memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = family;
        hints.ai_socktype = SOCK_STREAM;
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
            return get_addr_info_error(status);
        }

        /// Pointer to server info
        struct addrinfo *server{nullptr};
        /// String buffer used to store IP addresses
        char string_buffer[INET6_ADDRSTRLEN]{};
        /// Last registered error (if there is one) used for for-loop function uses
        /// continue rather than return
        ConnectStatus last_error{ConnectStatus::SUCCESS};

        // Loop through all found devices
        for (server = results; server != nullptr; server = server->ai_next)
        {
            // Creating endpoint for communication
            if ((_socket_params.fd = ::socket(server->ai_family,
                                              server->ai_socktype,
                                              server->ai_protocol)) == ERROR)
            {
                last_error = get_socket_error(errno);
                continue;
            }

            // Lose the pesky "Address already in use" error message
            if (::setsockopt(_socket_params.fd,
                             SOL_SOCKET,
                             SO_REUSEADDR,
                             &status,
                             sizeof(int)) == ERROR)
            {
                last_error = get_socket_option_error(errno);
                ::close(_socket_params.fd);
                continue;
            }

            // convert the IP to a string
            if (::inet_ntop(server->ai_family,
                            get_in_address(server->ai_addr, _socket_params.port),
                            string_buffer,
                            sizeof(string_buffer)) == nullptr)
            {
                last_error = get_inet_ntop_error(errno);
                ::close(_socket_params.fd);
                continue;
            }

            // Connecting to server
            if (::connect(_socket_params.fd, server->ai_addr, server->ai_addrlen) == ERROR)
            {
                last_error = get_connect_error(errno);
                ::close(_socket_params.fd);
                continue;
            }

            _socket_params.ip = string_buffer;
            _socket_params.family = convert_family(server->ai_family);

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
            return last_error;
        }

        if (_socket_params.fd == BAD_SOCKET)
            return last_error;

        return ConnectStatus::SUCCESS;
    }

    ConnectStatus Client::disconnect()
    {
        ConnectStatus status{ConnectStatus::SUCCESS};

        if (_socket_params.fd != BAD_SOCKET)
        {
            if (::close(_socket_params.fd) == ERROR)
            {
                status = get_close_error(errno);
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

        // Receiving over TCP
        total_received = ::recv(_socket_params.fd, buffer, size, NO_FLAGS);

        if (fill_receive_error(errno, total_received, status))
        {
            disconnect();
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
        return connect(_ip, _service, _family);
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

        // Sending over TCP
        total_sent = ::send(_socket_params.fd, buffer, size, MSG_NOSIGNAL);

        if (fill_send_error(errno, total_sent, status))
        {
            disconnect();
        }

        return total_sent;
    }
} // namespace: ramrod::socket