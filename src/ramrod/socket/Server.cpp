#include "ramrod/socket/Server.hpp"

#include <cerrno>       // for errno
#include <cstring>      // for memset
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
#include <sys/socket.h> // for recv, send, MSG_NOSIGNAL, accept
#include <sys/wait.h>   // for waitpid, WNOHANG
#include <unistd.h>     // for ssize_t, close

namespace {
    /// @brief Value indicating that socket is disconnected or if there was an error
    static constexpr int BAD_SOCKET{-1};
    /// @brief Value that indicates an error
    static constexpr int ERROR{-1};
}

namespace ramrod::socket
{
    Server::Server()
        : Conversor{},
          ip_{},
          ip_protocol_{},
          port_{},
          socket_type_{},
          socket_fd_{BAD_SOCKET},
          connected_fd_{BAD_SOCKET},
          connected_{false}
    {
    }

    Server::~Server()
    {
        disconnect();
    }

    bool Server::connect(const std::string &ip,
                         const std::uint16_t port,
                         const Protocol ip_protocol,
                         const SocketType socket_type)
    {
        if (connected_)
            return false;

        ip_ = ip;
        port_ = port;
        ip_protocol_ = ip_protocol;
        socket_type_ = socket_type;

        return connected_;
    }

    bool Server::disconnect()
    {

        connected_ = false;
        return true;
    }

    const std::string &Server::ip()
    {
        return ip_;
    }

    Protocol Server::ip_protocol()
    {
        return ip_protocol_;
    }

    bool Server::is_connected()
    {
        return connected_;
    }

    std::uint16_t Server::port()
    {
        return port_;
    }

    ssize_t Server::receive(void *buffer, const std::size_t size, const int flags)
    {
    }

    bool Server::reconnect()
    {
        return true;
    }

    ssize_t Server::send(const void *buffer, const std::size_t size, const int flags)
    {
    }

    SocketType Server::socket_type()
    {
        return socket_type_;
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
