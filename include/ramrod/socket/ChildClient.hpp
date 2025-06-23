#ifndef RAMROD_SOCKET_CHILD_CLIENT_HPP
#define RAMROD_SOCKET_CHILD_CLIENT_HPP

#include "ramrod/socket/BasicSocket.hpp"  // for BasicSocket
#include "ramrod/socket/Conversor.hpp"    // for Conversor
#include "ramrod/socket/Enumerators.hpp"  // for ErrorType, Family, SocketType
#include "ramrod/socket/ErrorHandler.hpp" // for ErrorHandler

#include <cstdint>     // for uint16_t
#include <string>      // for string
#include <sys/types.h> // for ssize_t

namespace ramrod::socket
{
    class ChildClient : public BasicSocket, public Conversor, public ErrorHandler
    {
    public:
        // TODO: fill
        ChildClient(const int fd,
                    const std::string &ip,
                    const std::uint16_t port,
                    const Family ip_family = Family::IPV4,
                    const SocketType socket_type = SocketType::STREAM);

        /**
         * @brief Call disconnect in destruction.
         */
        virtual ~ChildClient();

        /**
         * @brief Close this client's connection to server.
         *
         * You will not be able to send, nor receive data to/from server
         *
         * @return  It will return status when disconnecting, if an error is returned then,
         *          there is no need to call this again since disconnection is done regarless,
         *          it may serve for diagnostic purposes
         */
        ConnectStatus disconnect();

        /**
         * @brief Check if client is connected to server.
         *
         * @return True if connection is alive
         */
        bool is_connected();

        /**
         * @brief Receive data from server.
         *
         * @param[out] buffer  Output buffer where received data will be saved
         * @param[in] size     Is the number of bytes you want to receive
         * @param[out] status  If not nullptr then, this will be set with the receive status,
         *                     whenever return value is -1 this should have the failure cause
         *                     but, if everything goes well it should be equal to SUCCESS
         *
         * @return The number of bytes actually received, 0 when the server is disconnected or
         *         \p size is 0, or -1 on error and \p status should be different than SUCCESS.
         */
        ssize_t receive(void *buffer, const std::size_t size, ReceiveStatus *status = nullptr);

        /**
         * @brief Send data to server.
         *
         * @param[in] buffer   Is a pointer to the data you want to send
         * @param[in] size     Is the number of bytes you want to send
         * @param[out] status  If not nullptr then, this will be set with the receive status,
         *                     whenever return value is -1 this should have the failure cause
         *                     but, if everything goes well it should be equal to SUCCESS
         *
         * @return The number of bytes actually received, or 0 when the server is disconnected
         *         or if \p size is 0, or if is UDP and you have not yet received a packet to
         *         obtain client address information, or -1 on error and \p status should be
         *         different than SUCCESS.
         */
        ssize_t send(const void *buffer, const std::size_t size, SendStatus *status = nullptr);

    private:
        /// @brief Server address used to verify that received UDP data comes from server
        void *server_in_address_;
        /// @brief Real server address used to directly send UDP data to server
        void *server_real_address_;
        /// @brief Size of \p server_address_
        unsigned int server_address_length_;
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_CHILD_CLIENT_HPP
