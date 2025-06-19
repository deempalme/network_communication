#include "ramrod/socket/BasicSocket.hpp"

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

    SocketType BasicSocket::socket_type()
    {
        return _socket_params.type;
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