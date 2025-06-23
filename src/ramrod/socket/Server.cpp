#include "ramrod/socket/Server.hpp"

#include <algorithm> // for min
#include <arpa/inet.h>
#include <cerrno>       // for errno
#include <cstring>      // for memset
#include <limits>       // for numeric_limits
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
#include <sys/socket.h> // for accept, listen, bind, ...
#include <unistd.h>     // for close

namespace
{
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};

    /**
     * @brief Get IPv4 or IPv6 address from \b sockaddr.
     *
     * @param[in] sa     Sockect address structure returned from \b getaddrinfo()
     * @param[out] port  Will set this value from incoming \p sa port
     *
     * @return A void pointer to an IPv4's in_addr or IPv6's in6_addr compatible
     *         with \b inet_ntop()
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
    Server::Server()
        : BasicSocket{},
          Conversor{},
          ErrorHandler{},
          max_queue_count_{}
    {
    }

    Server::~Server()
    {
        close();
    }

    ConnectStatus Server::open(const std::uint16_t port,
                               const Family ip_family,
                               const SocketType socket_type,
                               const std::uint32_t queue)
    {
        if (_socket_params.fd != BAD_SOCKET)
        {
            return ConnectStatus::ALREADY_OPEN;
        }

        static constexpr uint16_t EMPTY_PORT{};
        const bool port_is_empty{port == EMPTY_PORT};
        if (port_is_empty)
        {
            return ConnectStatus::PORT_CANNOT_BE_EMPTY;
        }
        const std::string service{port_is_empty ? std::string{} : std::to_string(port)};

        return open(service, ip_family, socket_type, queue);
    }

    ConnectStatus Server::open(const std::string &service,
                               const Family ip_family,
                               const SocketType socket_type,
                               const std::uint32_t queue)
    {
        if (_socket_params.fd != BAD_SOCKET)
            return ConnectStatus::ALREADY_OPEN;

        if (service.empty())
            return ConnectStatus::SERVICE_CANNOT_BE_EMPTY;

        static constexpr std::uint32_t EMPTY_QUEUE{};
        if (queue == EMPTY_QUEUE)
            return ConnectStatus::QUEUE_FULL;

        int status{};

        _service = service;
        _family = ip_family;
        _socket_type = socket_type;
        /// Max integer value used to cap \p max_queue_count_
        static constexpr std::uint32_t MAX_INT_VALUE{
            static_cast<std::uint32_t>(std::numeric_limits<int>::max())};
        max_queue_count_ = static_cast<int>(std::min(MAX_INT_VALUE, queue));

        const int family{convert_family(ip_family)};

        struct addrinfo hints{};
        // make sure the struct is empty
        std::memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = family;
        hints.ai_socktype = convert_socket_type(socket_type);
        // fill in my IP for me
        hints.ai_flags = AI_PASSIVE;

        /// All found socket are stored in this list
        struct addrinfo *results{nullptr};

        /// Status indicating that socket's functions are working properly
        static constexpr int OK{};
        // Get all available devices that can be connected
        if ((status = ::getaddrinfo(nullptr, service.c_str(), &hints, &results)) != OK)
        {
            if (results != nullptr)
                ::freeaddrinfo(results);
            return get_addr_info_error(status);
        }

        /// Pointer to client info
        struct addrinfo *client{nullptr};
        /// String buffer used to store IP addresses
        char ip_string[INET6_ADDRSTRLEN]{};
        /// Last registered error (if there is one) used for for-loop function uses
        /// continue rather than return
        ConnectStatus last_error{ConnectStatus::SUCCESS};

        // Loop through all found devices
        for (client = results; client != nullptr; client = client->ai_next)
        {
            // Creating endpoint for communication
            if ((_socket_params.fd = ::socket(client->ai_family,
                                              client->ai_socktype,
                                              client->ai_protocol)) == ERROR)
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
            if (::inet_ntop(client->ai_family,
                            get_in_address(client->ai_addr, _socket_params.port),
                            ip_string,
                            INET6_ADDRSTRLEN) == nullptr)
            {
                last_error = get_inet_ntop_error(errno);
                ::close(_socket_params.fd);
                continue;
            }

            // Binding the socket to the port
            if (::bind(_socket_params.fd, client->ai_addr, client->ai_addrlen) == ERROR)
            {
                last_error = get_bind_error(errno);
                ::close(_socket_params.fd);
                continue;
            }

            _socket_params.ip = ip_string;
            _socket_params.family = convert_family(client->ai_family);
            _socket_params.type = convert_socket_type(client->ai_socktype);

            break;
        }

        // Free result's memory
        if (results != nullptr)
            ::freeaddrinfo(results);

        if (client == nullptr)
        {
            ::close(_socket_params.fd);
            _socket_params = {};
            _socket_params.fd = BAD_SOCKET;
            return last_error;
        }

        if ((_socket_params.fd == BAD_SOCKET) || // Not connected
            (socket_type != SocketType::STREAM)) // No need to listen if is datagram
        {
            return last_error;
        }

        if (::listen(_socket_params.fd, max_queue_count_) == ERROR)
        {
            return get_listen_error(errno);
        }

        return ConnectStatus::SUCCESS;
    }

    ConnectStatus Server::close()
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

    bool Server::is_open()
    {
        return _socket_params.fd != BAD_SOCKET;
    }

    std::shared_ptr<ChildClient> Server::accept(ConnectStatus *status)
    {
        std::shared_ptr<ChildClient> accepted_client{nullptr};

        if (_socket_params.fd == BAD_SOCKET)
        {
            if (status != nullptr)
                *status == ConnectStatus::NOT_CONNECTED;
            return accepted_client;
        }

        /// String buffer used to store IP addresses
        char buffer_string[INET6_ADDRSTRLEN]{};
        /// Address info of accepted client
        struct sockaddr_storage accepted_client_addr;
        /// Pointer to \p accepted_client_addr
        struct sockaddr *accepted_client_addr_ptr{
            reinterpret_cast<struct sockaddr *>(&accepted_client_addr)};
        /// Length of socket address structure
        socklen_t address_length{};
        /// File descriptor from accepted client
        int new_fd{BAD_SOCKET};

        if (_socket_params.type == SocketType::DATAGRAM)
        {
            // Waiting for the first message to arrive from a client
            ssize_t received_size{};
            received_size = ::recvfrom(_socket_params.fd,
                                       static_cast<void *>(&buffer_string),
                                       sizeof(buffer_string),
                                       MSG_PEEK,
                                       accepted_client_addr_ptr,
                                       &address_length);
            ReceiveStatus receive_status{};
            fill_receive_error(errno, received_size, &receive_status);

            if (receive_status != ReceiveStatus::SUCCESS)
            {
                if (status != nullptr)
                    *status = convert_to_connect_status(receive_status);
                return accepted_client;
            }
        }
        else if (_socket_params.type == SocketType::STREAM)
        {
            if ((new_fd = ::accept(_socket_params.fd,
                                   accepted_client_addr_ptr,
                                   &address_length)) == BAD_SOCKET)
            {
                const ConnectStatus accept_status{get_accept_error(errno)};
                if (accept_status == ConnectStatus::NOT_CONNECTED)
                    close();
                if (status != nullptr)
                    *status == accept_status;
                return accepted_client;
            }
        }

        /// Client's port
        std::uint16_t accepted_port{};
        /// Client's socket family
        const int client_addr_family{static_cast<int>(accepted_client_addr_ptr->sa_family)};

        // convert the IP to a string
        if (::inet_ntop(client_addr_family,
                        get_in_address(accepted_client_addr_ptr, accepted_port),
                        buffer_string,
                        INET6_ADDRSTRLEN) == nullptr)
        {
            if (status != nullptr)
                *status == get_inet_ntop_error(errno);
            ::close(new_fd);
            return accepted_client;
        }

        if (status != nullptr)
            *status == ConnectStatus::SUCCESS;

        const std::string accepted_ip{buffer_string};
        const Family accepted_family{convert_family(client_addr_family)};
        const SocketType accepted_socket_type{_socket_params.type};

        accepted_client = std::make_shared<ChildClient>(ChildClient{new_fd,
                                                                    accepted_ip,
                                                                    accepted_port,
                                                                    accepted_family,
                                                                    accepted_socket_type});

        return accepted_client;
    }

    ConnectStatus Server::reopen()
    {
        close();

        if (!is_initialized())
        {
            return ConnectStatus::OPEN_HAS_NOT_BEEN_CALLED_YET;
        }
        return open(_service, _family, _socket_type, max_queue_count_);
    }
} // namespace: ramrod::socket
