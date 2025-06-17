#include "ramrod/socket/Server.hpp"

#include <arpa/inet.h>
#include <cerrno>       // for errno
#include <cstring>      // for memset
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
#include <sys/socket.h> // for recv, send, MSG_NOSIGNAL, accept
#include <sys/wait.h>   // for waitpid, WNOHANG
#include <unistd.h>     // for ssize_t, close

namespace
{
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};

    /**
     * @brief Get ErrorType from getaddrinfo() returned status.
     *
     * @param[in] get_addr_info_status  Status returned by getaddrinfo()
     *
     * @return An error enum compatible with Server class
     */
    ramrod::socket::ErrorType get_error_type(const int get_addr_info_status)
    {
        using namespace ramrod::socket;

        switch (get_addr_info_status)
        {
        case EAI_BADFLAGS:
            return ErrorType::ADDRESS_INFO_BAD_FLAGS;
        case EAI_FAMILY:
            return ErrorType::ADDRESS_INFO_FAMILY_NOT_SUPPORTED;
        case EAI_NODATA:
            return ErrorType::ADDRESS_INFO_NO_ADDRESS_DEFINED;
        case EAI_NONAME:
            return ErrorType::ADDRESS_INFO_NO_NAME;
        case EAI_MEMORY:
            return ErrorType::ADDRESS_INFO_OUT_OF_MEMORY;
        case EAI_SERVICE:
            return ErrorType::ADDRESS_INFO_SERVICE_NOT_AVAILABLE;
        case EAI_SOCKTYPE:
            return ErrorType::ADDRESS_INFO_SOCKET_TYPE_NOT_SUPPORTED;
        case EAI_SYSTEM:
            return ErrorType::ADDRESS_INFO_SYSTEM_ERROR;
        case EAI_AGAIN:
            return ErrorType::ADDRESS_INFO_TRY_AGAIN_LATER;
        case EAI_ADDRFAMILY:
            return ErrorType::ADDRESS_INFO_UNKNOWN_ADDRESS_FAMILY;
        default:
            return ErrorType::ADDRESS_INFO_PERMANENT_FAILURE;
        }
    }

    /**
     * @brief Convert socket::Family into standard family.
     *
     * @param[in] family  IP Family version taken from socket::Family
     *
     * @return Standard IP family version
     */
    int get_family(const ramrod::socket::Family family)
    {
        using namespace ramrod::socket;

        switch (family)
        {
        case Family::IPV4:
            return AF_INET;
        case Family::IPV6:
            return AF_INET6;
        default:
            return AF_UNSPEC;
        }
    }

    /**
     * @brief Convert standard family into socket::Family.
     *
     * @param[in] family  Standard IP Family version taken from socket
     *
     * @return socket::Family's IP family version
     */
    ramrod::socket::Family get_family(const int family)
    {
        using namespace ramrod::socket;

        switch (family)
        {
        case AF_INET:
            return Family::IPV4;
        case AF_INET6:
            return Family::IPV6;
        default:
            return Family::UNSPECIFIED;
        }
    }

    /**
     * @brief Convert socket::SocketType into standard socket type.
     *
     * @param[in] type  Socket type
     *
     * @return Standard socket type
     */
    int get_socket_type(const ramrod::socket::SocketType type)
    {
        using namespace ramrod::socket;

        switch (type)
        {
        case SocketType::DATAGRAM:
            return SOCK_DGRAM;
        case SocketType::STREAM:
            return SOCK_STREAM;
        default:
            return ERROR;
        }
    }

    /**
     * @brief Convert standard socket type into socket::SocketType.
     *
     * @param[in] type  Standard socket type taken from socket
     *
     * @return socket::Family's socket type
     */
    ramrod::socket::SocketType get_socket_type(const int type)
    {
        using namespace ramrod::socket;

        switch (type)
        {
        case SOCK_DGRAM:
            return SocketType::DATAGRAM;
        case SOCK_STREAM:
        default:
            return SocketType::STREAM;
        }
    }
}

namespace ramrod::socket
{
    Server::Server()
        : Conversor{},
          target_ip_{},
          target_family_{},
          target_port_{},
          target_socket_type_{},
          active_socket_{BAD_SOCKET},
          last_error_{},
          last_error_code_{}
    {
    }

    Server::~Server()
    {
        disconnect();
    }

    bool Server::connect(const std::string &ip,
                         const std::uint16_t port,
                         const Family ip_family,
                         const SocketType socket_type)
    {
        if (active_socket_.fd != BAD_SOCKET)
        {
            last_error_ = ErrorType::ALREADY_CONNECTED;
            return false;
        }

        int status{};

        target_ip_ = ip;
        target_port_ = port;
        target_family_ = ip_family;
        target_socket_type_ = socket_type;

        const int family{get_family(ip_family)};
        const in_port_t network_port{::htons(port)};

        struct addrinfo hints{};
        // make sure the struct is empty
        std::memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = family;
        hints.ai_socktype = get_socket_type(socket_type);
        // fill in my IP for me
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *results{nullptr};

        /// Status indicating that socket's functions are working properly
        static constexpr int OK{};
        const char *node{target_ip_.empty() ? nullptr : target_ip_.c_str()};
        static constexpr uint16_t EMPTY_PORT{};
        const char *service{port == EMPTY_PORT ? nullptr : std::to_string(port).c_str()};

        // Get all available devices that can be connected
        if ((status = ::getaddrinfo(node, service, &hints, &results)) != OK)
        {
            last_error_ = get_error_type(status);
            last_error_code_ = status;
            if (results != nullptr)
                ::freeaddrinfo(results);
            return false;
        }

        struct addrinfo *client{nullptr};
        /// Socket's IP string
        char socket_ip_string[INET6_ADDRSTRLEN];

        // Loop through all found devices
        for (client = results; client != nullptr; client = client->ai_next)
        {
            // Creating endpoint for communication
            if ((active_socket_.fd = ::socket(client->ai_family,
                                              client->ai_socktype,
                                              client->ai_protocol)) == ERROR)
            {
                last_error_ = ErrorType::CREATE_SOCKET_ERROR;
                last_error_code_ = errno;
                continue;
            }

            void *addr;
            struct sockaddr_in *ipv4;
            struct sockaddr_in6 *ipv6;

            // get the pointer to the address itself,
            // different fields in IPv4 and IPv6:
            if (client->ai_family == AF_INET)
            {
                // IPv4
                ipv4 = reinterpret_cast<struct sockaddr_in *>(client->ai_addr);
                addr = &ipv4->sin_addr;
                active_socket_.port = ipv4->sin_port;
            }
            else
            {
                // IPv6
                ipv6 = reinterpret_cast<struct sockaddr_in6 *>(client->ai_addr);
                addr = &ipv6->sin6_addr;
                active_socket_.port = ipv6->sin6_port;
            }

            // convert the IP to a string
            if (::inet_ntop(client->ai_family,
                            addr,
                            socket_ip_string,
                            sizeof(socket_ip_string)) == nullptr)
            {
                last_error_ = ErrorType::IP_CONVERSION_FAILED;
                last_error_code_ = errno;
                ::close(active_socket_.fd);
                continue;
            }

            // Lose the pesky "Address already in use" error message
            if (::setsockopt(active_socket_.fd,
                             SOL_SOCKET,
                             SO_REUSEADDR,
                             &status,
                             sizeof(int)) == ERROR)
            {
                last_error_ = ErrorType::SET_SOCKET_OPTION_ERROR;
                last_error_code_ = errno;
                ::close(active_socket_.fd);
                continue;
            }

            // Binding the socket to the port
            if (::bind(active_socket_.fd, client->ai_addr, client->ai_addrlen) == ERROR)
            {
                last_error_ = ErrorType::BIND_SOCKET_ERROR;
                last_error_code_ = errno;
                ::close(active_socket_.fd);
                continue;
            }

            active_socket_.ip = socket_ip_string;
            active_socket_.family = get_family(client->ai_family);
            active_socket_.type = get_socket_type(client->ai_socktype);

            break;
        }

        // Free result's memory
        if (results != nullptr)
            ::freeaddrinfo(results);

        return active_socket_.fd != BAD_SOCKET;
    }

    bool Server::disconnect()
    {
        bool ok{true};

        if (active_socket_.fd != BAD_SOCKET)
        {
            if (::shutdown(active_socket_.fd, SHUT_RDWR) == ERROR)
            {
                last_error_ = ErrorType::SHUTDOWN_ERROR;
                last_error_code_ = errno;
                ok = false;
            }
            if (::close(active_socket_.fd) == ERROR)
            {
                last_error_ = ErrorType::CLOSE_ERROR;
                last_error_code_ = errno;
                ok = false;
            }
            active_socket_ = {};
            active_socket_.fd = BAD_SOCKET;
        }

        return ok;
    }

    const std::string &Server::ip()
    {
        return active_socket_.ip;
    }

    Family Server::ip_family()
    {
        return active_socket_.family;
    }

    bool Server::is_connected()
    {
        // TODO: maybe check if there are connections
        return active_socket_.fd != BAD_SOCKET;
    }

    ErrorType Server::last_error()
    {
        return last_error_;
    }

    const char *Server::last_error_detail()
    {
        switch (last_error_)
        {
        case ErrorType::SUCCESS:
            static constexpr char SUCCESS_MSG[]{"No error encountered"};
            return SUCCESS_MSG;
        case ErrorType::ALREADY_CONNECTED:
            static constexpr char ALREADY_CONNECTED_MSG[]{"There is already an active connection"};
            return ALREADY_CONNECTED_MSG;
        case ErrorType::ADDRESS_INFO_BAD_FLAGS:
        case ErrorType::ADDRESS_INFO_FAMILY_NOT_SUPPORTED:
        case ErrorType::ADDRESS_INFO_NO_ADDRESS_DEFINED:
        case ErrorType::ADDRESS_INFO_NO_NAME:
        case ErrorType::ADDRESS_INFO_OUT_OF_MEMORY:
        case ErrorType::ADDRESS_INFO_PERMANENT_FAILURE:
        case ErrorType::ADDRESS_INFO_SERVICE_NOT_AVAILABLE:
        case ErrorType::ADDRESS_INFO_SOCKET_TYPE_NOT_SUPPORTED:
        case ErrorType::ADDRESS_INFO_TRY_AGAIN_LATER:
        case ErrorType::ADDRESS_INFO_UNKNOWN_ADDRESS_FAMILY:
            return ::gai_strerror(last_error_code_);
        case ErrorType::ADDRESS_INFO_SYSTEM_ERROR:
        case ErrorType::BIND_SOCKET_ERROR:
        case ErrorType::CLOSE_ERROR:
        case ErrorType::CREATE_SOCKET_ERROR:
        case ErrorType::IP_CONVERSION_FAILED:
        case ErrorType::SET_SOCKET_OPTION_ERROR:
        case ErrorType::SHUTDOWN_ERROR:
        case ErrorType::SYSTEM_ERROR:
            return std::strerror(last_error_code_);
        default:
            static constexpr char UNKNOWN_ERROR_MSG[]{"Unknown error"};
            return UNKNOWN_ERROR_MSG;
        }
    }

    std::uint16_t Server::port()
    {
        return active_socket_.port;
    }

    ssize_t Server::receive(void *buffer, const std::size_t size, const int flags)
    {
    }

    bool Server::reconnect()
    {
        disconnect();
        return connect(target_ip_, target_port_, target_family_, target_socket_type_);
    }

    ssize_t Server::send(const void *buffer, const std::size_t size, const int flags)
    {
    }

    SocketType Server::socket_type()
    {
        return active_socket_.type;
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    // :::::::::::::::::::::::::::::::::::: OUTTER FUNCTIONS :::::::::::::::::::::::::::::::::::

    void signal_children_handler(const int /*signal*/)
    {
        // waitpid() might overwrite errno, so we save and restore it:
        const int saved_errno = errno;
        while (::waitpid(-1, nullptr, WNOHANG) > 0)
            ;
        errno = saved_errno;
    }
} // namespace: ramrod::socket
