#include "ramrod/socket/BasicSocket.hpp"

#include <algorithm>    // for equal
#include <netdb.h>      // for INET6_ADDRSTRLEN
#include <sys/socket.h> // for AF_INET, AF_INET6, ...

namespace
{
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};
}

namespace ramrod::socket
{
    BasicSocket::BasicSocket()
        : _ip{},
          _family{Family::UNSPECIFIED},
          _service{},
          _socket_type{SocketType::STREAM},
          _socket_params{BAD_SOCKET}
    {
    }

    const std::string &BasicSocket::ip()
    {
        return _socket_params.ip;
    }

    Family BasicSocket::ip_family()
    {
        return _socket_params.family;
    }

    std::uint16_t BasicSocket::port()
    {
        return _socket_params.port;
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

    SocketType BasicSocket::socket_type()
    {
        return _socket_params.type;
    }

    // ::::::::::::::::::::::::::::::::::: PROTECTED FUNCTIONS :::::::::::::::::::::::::::::::::::

    bool BasicSocket::are_addresses_equal(const void *a, const void *b)
    {
        const struct sockaddr *a_ptr{static_cast<const struct sockaddr *>(a)};

        // Checking if they have the same type
        if (a_ptr->sa_family != static_cast<const struct sockaddr *>(b)->sa_family)
            return false;

        if (a_ptr->sa_family == AF_INET)
        {
            const struct sockaddr_in *a_in{static_cast<const struct sockaddr_in *>(a)};
            const struct sockaddr_in *b_in{static_cast<const struct sockaddr_in *>(b)};
            // Checking that both address and port are equals
            return (a_in->sin_addr.s_addr == b_in->sin_addr.s_addr) &&
                   (a_in->sin_port == b_in->sin_port);
        }
        else if (a_ptr->sa_family == AF_INET6)
        {
            const struct sockaddr_in6 *a_in6{static_cast<const struct sockaddr_in6 *>(a)};
            const struct sockaddr_in6 *b_in6{static_cast<const struct sockaddr_in6 *>(b)};
            // Checking that both address (byte by byte) and port are equals
            return std::equal(std::begin(a_in6->sin6_addr.__in6_u.__u6_addr32),
                              std::end(a_in6->sin6_addr.__in6_u.__u6_addr32),
                              std::begin(b_in6->sin6_addr.__in6_u.__u6_addr32)) &&
                   (a_in6->sin6_port == b_in6->sin6_port);
        }
        // Other families are not supported
        return false;
    }

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

    int BasicSocket::convert_socket_type(const ramrod::socket::SocketType type)
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

    SocketType BasicSocket::convert_socket_type(const int type)
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

    bool BasicSocket::is_initialized()
    {
        // At least one value must be set
        return (_family == Family::UNSPECIFIED) &&
               (_service.empty() && _ip.empty()) &&
               (_socket_type == SocketType::STREAM);
    }
} // namespace: ramrod::socket