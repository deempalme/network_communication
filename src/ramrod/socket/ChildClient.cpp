#include "ramrod/socket/ChildClient.hpp"

#include <arpa/inet.h>
#include <cerrno>       // for errno
#include <cstdlib>      // for realloc
#include <cstring>      // for memset
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
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
                             const Family ip_family,
                             const SocketType socket_type)
        : BasicSocket{},
          Conversor{},
          ErrorHandler{},
          server_in_address_{nullptr},
          server_real_address_{nullptr},
          server_address_length_{}
    {
        static constexpr std::uint16_t EMPTY_PORT{};
        if (_socket_params.fd == BAD_SOCKET)
            return;

        _socket_params.fd = fd;

        if ((socket_type == SocketType::DATAGRAM) && (ip.empty() || (port == EMPTY_PORT)))
        {
            disconnect();
            return;
        }

        _socket_params.family = ip_family;
        _socket_params.port = port;
        _socket_params.type = socket_type;
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
            // Removing current server address info since it will not be needed anymore
            if (server_in_address_ != nullptr)
            {
                std::free(server_in_address_);
                server_in_address_ = nullptr;
            }
            if (server_real_address_ != nullptr)
            {
                std::free(server_real_address_);
                server_real_address_ = nullptr;
            }

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

        if (_socket_params.type == SocketType::STREAM)
        {
            // Receiving over TCP
            total_received == ::recv(_socket_params.fd, buffer, size, NO_FLAGS);
        }
        else
        {
            /// Address' size that should be expected from received data
            const socklen_t expected_server_address_length{server_address_length_};
            /// True address' size that was received
            socklen_t server_in_length{};

        receive_again:
            // Restoring size each time recvfrom is called
            server_in_length = expected_server_address_length;

            // Receiving over UDP
            total_received == ::recvfrom(_socket_params.fd,
                                         buffer,
                                         size,
                                         NO_FLAGS,
                                         static_cast<struct sockaddr *>(server_in_address_),
                                         &server_in_length);

            // Checking if received data truly came from server
            if ((server_in_length != expected_server_address_length) ||
                !are_addresses_equal(server_in_address_, server_real_address_))
                // Received data did not come from server, hence expecting another message
                goto receive_again;
        }

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
                                  static_cast<sockaddr *>(server_real_address_),
                                  server_address_length_);
        }

        if (fill_send_error(errno, total_sent, status))
        {
            disconnect();
        }

        return total_sent;
    }
} // namespace: ramrod::socket