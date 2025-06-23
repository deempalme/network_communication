#include "ramrod/socket/BasicSocket.hpp"

#include <arpa/inet.h>      // for inet_ntop
#include <bits/local_lim.h> // for HOST_NAME_MAX
#include <cerrno>           // for errno, EBADF, EINVAL, ENOTCONN, ENOTSOCK
#include <cstring>          // for memset
#include <netdb.h>          // for addrinfo, freeaddrinfo, getaddrinfo, AI_...
#include <netinet/in.h>     // for INET6_ADDRSTRLEN, sockaddr_in, sockaddr_in6
#include <signal.h>         // for sigaction, SA_RESTART, SIGCHLD, sigemptyset
#include <sys/socket.h>     // for AF_INET, AF_INET6, AF_UNSPEC, shutdown
#include <sys/wait.h>       // for waitpid, WNOHANG
#include <unistd.h>         // for gethostname

namespace
{
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};

    /**
     * @brief Reap dead processes.
     */
    void signal_children_handler(const int /*signal*/)
    {
        /// waitpid() might overwrite errno, so we save and restore it:
        const int saved_errno = errno;
        while (::waitpid(-1, nullptr, WNOHANG) > 0)
            ;
        errno = saved_errno;
    }

    /**
     * @brief Get IPv4 or IPv6 address from \b sockaddr.
     *
     * @param[in] sa Sockect address structure returned from \b getaddrinfo()
     *
     * @return A void pointer to an IPv4's in_addr or IPv6's in6_addr compatible
     *         with \b inet_ntop()
     */
    void *get_in_address(struct sockaddr *sa)
    {
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (sa->sa_family == AF_INET)
        {
            // IPv4
            ipv4 = reinterpret_cast<struct sockaddr_in *>(sa);
            return static_cast<void *>(&ipv4->sin_addr);
        }

        // IPv6
        ipv6 = reinterpret_cast<struct sockaddr_in6 *>(sa);
        return static_cast<void *>(&ipv6->sin6_addr);
    }
}

namespace ramrod::socket
{
    BasicSocket::BasicSocket()
        : _ip{},
          _family{Family::UNSPECIFIED},
          _service{},
          _socket_params{BAD_SOCKET},
          _device_hostname{},
          _device_ip4{},
          _device_ip6{}
    {
        static_assert(HOST_NAME_MAX > INET6_ADDRSTRLEN,
                      "Hostname max size cannot be smaller than IPv6 max size");

        char string_buffer[HOST_NAME_MAX]{};

        if (::gethostname(string_buffer, HOST_NAME_MAX) == ERROR)
            return;

        // Making sure hostname is truncated
        string_buffer[HOST_NAME_MAX - 1ul] = '\0';

        _device_hostname = string_buffer;

        struct addrinfo hints{};
        // make sure the struct is empty
        std::memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        // fill in my IP for me
        hints.ai_flags = AI_PASSIVE;

        struct addrinfo *results{nullptr};

        /// Status indicating that socket's functions are working properly
        static constexpr int OK{};
        // Get all available IPs from device
        if (::getaddrinfo(string_buffer, nullptr, &hints, &results) != OK)
        {
            if (results != nullptr)
                ::freeaddrinfo(results);
            return;
        }

        struct addrinfo *ip{nullptr};
        // Going through all found IPs
        for (ip = results; ip != nullptr; ip = ip->ai_next)
        {
            const bool is_v4{ip->ai_family == AF_INET};
            if (is_v4 || (ip->ai_family == AF_INET6))
            {
                // convert the IP to a string
                if (::inet_ntop(ip->ai_family,
                                get_in_address(ip->ai_addr),
                                string_buffer,
                                INET6_ADDRSTRLEN) == nullptr)
                {
                    continue;
                }
                if (is_v4)
                    _device_ip4 = string_buffer;
                else
                    _device_ip6 = string_buffer;
            }
        }
        if (results != nullptr)
            ::freeaddrinfo(results);
    }

    const std::string &BasicSocket::hostname()
    {
        return _device_hostname;
    }

    const std::string &BasicSocket::ip(const Family family)
    {
        if (family == Family::IPV6)
            return _device_ip6;
        else if (family == Family::IPV4)
            return _device_ip4;

        return _socket_params.family == Family::IPV6 ? _device_ip6 : _device_ip4;
    }

    Family BasicSocket::ip_family()
    {
        return _socket_params.family;
    }

    std::uint16_t BasicSocket::port()
    {
        return _socket_params.port;
    }

    bool BasicSocket::read_dead_processes(const bool stop)
    {
        /// Signal action to reap all dead processes
        static struct sigaction reap_signal_action{};
        /// Signal action containing previos action
        static struct sigaction old_signal_action{};
        /// Check if signal action has already been initialized
        const bool initialized{reap_signal_action.sa_flags == SA_RESTART};

        if (stop && initialized)
        {
            // Stoping reaping dead child processes
            reap_signal_action = {};
            return ::sigaction(SIGCHLD, &old_signal_action, nullptr) != ERROR;
        }

        // Connecting signal only once
        if (!initialized)
        {
            reap_signal_action.sa_handler = signal_children_handler;
            ::sigemptyset(&reap_signal_action.sa_mask);
            reap_signal_action.sa_flags = SA_RESTART;
            return ::sigaction(SIGCHLD, &reap_signal_action, &old_signal_action) != ERROR;
        }
        return true;
    }

    ConnectStatus BasicSocket::shutdown(const ShutdownType type)
    {
        if (_socket_params.fd == BAD_SOCKET)
        {
            return ConnectStatus::NOT_CONNECTED;
        }

        int how_to_shutdown{};
        switch (type)
        {
        case ShutdownType::RECEIVE:
            how_to_shutdown = SHUT_RD;
            break;
        case ShutdownType::SEND:
            how_to_shutdown = SHUT_WR;
            break;
        default:
            how_to_shutdown = SHUT_RDWR;
            break;
        }

        if (::shutdown(_socket_params.fd, how_to_shutdown) == ERROR)
        {
            switch (errno)
            {
            case EBADF:
            case ENOTCONN:
            case ENOTSOCK:
                return ConnectStatus::NOT_CONNECTED;
            case EINVAL:
            default:
                return ConnectStatus::UNKNOWN_ERROR;
            }
        }

        return ConnectStatus::SUCCESS;
    }

    // ::::::::::::::::::::::::::::::::::: PROTECTED FUNCTIONS :::::::::::::::::::::::::::::::::::

    int BasicSocket::convert_family(const ramrod::socket::Family family)
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

    Family BasicSocket::convert_family(const int family)
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

    bool BasicSocket::is_initialized()
    {
        // At least one value must be set
        return (_family == Family::UNSPECIFIED) &&
               (_service.empty() && _ip.empty());
    }
} // namespace: ramrod::socket