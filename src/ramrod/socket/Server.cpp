#include "ramrod/socket/Server.hpp"

#include <arpa/inet.h>
#include <cerrno>       // for errno
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

    /**
     * @brief Reap dead processes.
     */
    void signal_children_handler(const int /*signal*/)
    {
        // waitpid() might overwrite errno, so we save and restore it:
        const int saved_errno = errno;
        while (::waitpid(-1, nullptr, WNOHANG) > 0)
            ;
        errno = saved_errno;
    }
} // Unnamed namespace

namespace ramrod::socket
{
    Server::Server()
        : BasicSocket{},
          Conversor{},
          ErrorHandler{}
    {
        // Creating a signal connection to reap all dead processes
        struct sigaction signal_action;
        signal_action.sa_handler = signal_children_handler;
        ::sigemptyset(&signal_action.sa_mask);
        signal_action.sa_flags = SA_RESTART;
        if (::sigaction(SIGCHLD, &signal_action, nullptr) == ERROR)
        {
            set_error_code(ErrorType::DEAD_PROCESSES_REAPING_CONNECTION_FAILED, errno);
        }
    }

    Server::~Server()
    {
        close();
    }

    ErrorType Server::open(const std::uint16_t port,
                           const Family ip_family,
                           const SocketType socket_type)
    {
        if (_socket_params.fd != BAD_SOCKET)
        {
            return ErrorType::ALREADY_OPEN;
        }

        static constexpr uint16_t EMPTY_PORT{};
        if (port == EMPTY_PORT)
        {
            return ErrorType::PORT_CANNOT_BE_EMPTY;
        }
        const std::string service{std::to_string(port)};

        return open(service, ip_family, socket_type);
    }

    ErrorType Server::open(const std::string &service,
                           const Family ip_family,
                           const SocketType socket_type)
    {
        if (_socket_params.fd != BAD_SOCKET)
        {
            return ErrorType::ALREADY_OPEN;
        }

        if (service.empty())
        {
            return ErrorType::SERVICE_CANNOT_BE_EMPTY;
        }

        int status{};

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

        /// All found socket are stored in this list
        struct addrinfo *results{nullptr};

        /// Status indicating that socket's functions are working properly
        static constexpr int OK{};
        // Get all available devices that can be connected
        if ((status = ::getaddrinfo(nullptr, service.c_str(), &hints, &results)) != OK)
        {
            if (results != nullptr)
                ::freeaddrinfo(results);
            return set_error_code(status);
        }

        /// Pointer to client info
        struct addrinfo *client{nullptr};
        /// Socket's IP string
        char socket_ip_string[INET6_ADDRSTRLEN];
        /// Last registered error (if there is one) used for for-loop function uses
        /// continue rather than return
        int last_error_code{};

        // Loop through all found devices
        for (client = results; client != nullptr; client = client->ai_next)
        {
            // Creating endpoint for communication
            if ((_socket_params.fd = ::socket(client->ai_family,
                                              client->ai_socktype,
                                              client->ai_protocol)) == ERROR)
            {
                last_error_code = errno;
                set_error_code(ErrorType::CREATE_SOCKET_ERROR, errno);
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
                ::close(_socket_params.fd);
                continue;
            }

            // convert the IP to a string
            if (::inet_ntop(client->ai_family,
                            get_in_address(client->ai_addr, _socket_params.port),
                            socket_ip_string,
                            sizeof(socket_ip_string)) == nullptr)
            {
                last_error_code = errno;
                set_error_code(ErrorType::IP_CONVERSION_FAILED, errno);
                ::close(_socket_params.fd);
                continue;
            }

            // Binding the socket to the port
            if (::bind(_socket_params.fd, client->ai_addr, client->ai_addrlen) == ERROR)
            {
                last_error_code = errno;
                set_error_code(ErrorType::BIND_SOCKET_ERROR, errno);
                ::close(_socket_params.fd);
                continue;
            }

            _socket_params.ip = socket_ip_string;
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
            return set_error_code(ErrorType::NO_SOCKET_AVAILABLE, last_error_code);
        }

        if (_socket_params.fd == BAD_SOCKET)
        {
            return set_error_code(ErrorType::NO_SOCKET_AVAILABLE, last_error_code);
        }

        return ErrorType::SUCCESS;
    }

    ErrorType Server::close()
    {
        ErrorType status{ErrorType::SUCCESS};

        if (_socket_params.fd != BAD_SOCKET)
        {
            if (::close(_socket_params.fd) == ERROR)
            {
                status = set_error_code(ErrorType::CLOSE_ERROR, errno);
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

    ErrorType Server::reopen()
    {
        close();

        if (!is_initialized())
        {
            return ErrorType::OPEN_HAS_NOT_BEEN_CALLED_YET;
        }
        return open(_service, _family, _socket_type);
    }
} // namespace: ramrod::socket
