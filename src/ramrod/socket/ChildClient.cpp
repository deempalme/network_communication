#include "ramrod/socket/ChildClient.hpp"

#include <cerrno>       // for errno
#include <sys/socket.h> // for recv, send, MSG_NOSIGNAL, accept
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
} // Unnamed namespace

namespace ramrod::socket
{
    ChildClient::ChildClient(const int fd,
                             const std::string &ip,
                             const std::uint16_t port,
                             const Family ip_family)
        : BasicSocket{},
          Conversor{},
          ErrorHandler{}
    {
        static constexpr std::uint16_t EMPTY_PORT{};
        if (_socket_params.fd == BAD_SOCKET)
            return;

        _socket_params.fd = fd;

        if (ip.empty() || (port == EMPTY_PORT))
        {
            disconnect();
            return;
        }

        _socket_params.family = ip_family;
        _socket_params.port = port;
        _socket_params.ip = ip;
    }

    ChildClient::~ChildClient()
    {
        disconnect();
    }

    ConnectStatus ChildClient::disconnect()
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

    bool ChildClient::is_connected()
    {
        return _socket_params.fd != BAD_SOCKET;
    }

    ssize_t ChildClient::receive(void *buffer, const std::size_t size, ReceiveStatus *status)
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

    ssize_t ChildClient::send(const void *buffer, const std::size_t size, SendStatus *status)
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