#ifndef RAMROD_NETWORK_COMMUNICATION_CLIENT_H
#define RAMROD_NETWORK_COMMUNICATION_CLIENT_H

#include <cstdint>       // for uint32_t, uint16_t
#include <iosfwd>        // for size_t
#include <string>        // for string
#include <sys/socket.h>  // for recv, send, MSG_NOSIGNAL, accept

#include "ramrod/network_communication/conversor.h"  // for conversor


namespace ramrod {
  namespace network_communication {
    class client : public conversor
    {
    public:
      client();
      ~client();
      enum error_value : int {
        /// Everything went well
        no_error               = 0,
        /// IP address or Port have not been set up
        port_or_ip_not_defined = 1,
        /// Client is already tryin to connect
        already_connecting     = 2,
        /// The number of re/connection intents have been reached
        reached_max_intents    = 3,
        /// Error in ::getaddinfo(), the error code was saved into errno and you could get
        /// detailed error description using ::gai_strerror(errno)
        getaddrinfo_error      = 4
      };
      /**
       * @brief Makes a TCP socket stream connection to an specific IP and port
       *
       * This will disconnect any previous connection and it will create a one with the new
       * parameters
       *
       * @param ip                  Selected IP address to connect
       * @param port                Port number to where the connection will be made
       * @param socket_type         Defines the type of connection TCP or UDP, options:
       *                              SOCK_STREAM   Creates a TCP socket
       *                              SOCK_DGRAM    Creates a connected datagram socket
       * @param timeout             Maximum number of milliseconds that system should wait for
       *                            a successfull connection before is dropped
       * @param connection_intents  Maximum number of intents to connect to server
       *
       * @return 0 if successful, -1 if there is an error (errno is set),
       *         or an error_value is returned
       */
      int connect(const std::string &ip, const int port,
                  const int socket_type = SOCK_STREAM, const int timeout = -1,
                  const int connection_intents = 1);
      /**
       * @brief Indicates if the client is currently trying to connect to the server
       *
       * @return `true` if is actually trying to connect to the server
       */
      bool connecting();
      /**
       * @brief Disconnects this device from the current connected network's device
       *
       * @return 0 if connection was correctly closed else gives the errno
       */
      int disconnect();
      /**
       * @brief Getting the current IP address
       *
       * @return String containing the IP address
       */
      const std::string& ip();
      /**
       * @brief Indicates if there is connection with another device
       *
       * @return `true` if there is an open connection
       */
      bool is_connected();
      /**
       * @brief Getting the current port
       *
       * @return Port's value
       */
      int port();
      /**
       * @brief Receives data from a TCP socket stream
       *
       * @param buffer Is a pointer to the data you want to receive
       * @param size   Is the number of bytes you want to receive
       * @param flags  Allows you to specify more information about how the data is to be received.
       *          MSG_OOB      Receive as “out of band” data. This is how to get data that has
       *                       been sent to you with the `MSG_OOB` flag in `send()`. As the
       *                       receiving side, you will have had signal `SIGURG` raised telling
       *                       you there is urgent data. In your handler for that signal, you
       *                       could call `receive()` with this `MSG_OOB` flag.
       *          MSG_PEEK     If you want to call `receive()` “just for pretend”, you can call
       *                       it with this flag. This will tell you what’s waiting in the
       *                       buffer for when you call `receive()` “for real” (i.e. without
       *                       the `MSG_PEEK` flag. It’s like a sneak preview into the next
       *                       `receive()` call.
       *          MSG_WAITALL  Tell `receive()` to not return until all the data you specified
       *                       in the len parameter. It will ignore your wishes in extreme
       *                       circumstances, however, like if a signal interrupts the call
       *                       or if some error occurs or if the remote side closes the
       *                       connection, etc. Don’t be mad with it.
       *
       * @return The number of bytes actually received, or 0 when the server is disconnected,
       *         or -1 on error (and `errno` will be set accordingly).
       */
      ssize_t receive(void *buffer, const std::size_t size, const int flags = 0);
      /**
       * @brief Receives all required sized data from a TCP socket stream
       *
       * This will loop until all the specified data's size has been received
       *
       * @param buffer  Is a pointer to the data you want to receive
       * @param size    Is the number of bytes you want to receive
       * @param flags   Allows you to specify more information about how the data is to be received.
       *          MSG_OOB      Receive as “out of band” data. This is how to get data that has
       *                       been sent to you with the `MSG_OOB` flag in `send()`. As the
       *                       receiving side, you will have had signal `SIGURG` raised telling
       *                       you there is urgent data. In your handler for that signal, you
       *                       could call `receive()` with this `MSG_OOB` flag.
       *          MSG_PEEK     If you want to call `receive()` “just for pretend”, you can call
       *                       it with this flag. This will tell you what’s waiting in the
       *                       buffer for when you call `receive()` “for real” (i.e. without
       *                       the `MSG_PEEK` flag. It’s like a sneak preview into the next
       *                       `receive()` call.
       *          MSG_WAITALL  Tell `receive()` to not return until all the data you specified
       *                       in the len parameter. It will ignore your wishes in extreme
       *                       circumstances, however, like if a signal interrupts the call
       *                       or if some error occurs or if the remote side closes the
       *                       connection, etc. Don’t be mad with it.
       *
       * @return The number of bytes actually received, or 0 when the server is disconnected,
       *         or -1 on error (and `errno` will be set accordingly).
       */
      ssize_t receive_all(void *buffer, const std::size_t size, const int flags = 0);
      /**
       * @brief Reconnecting again
       *
       * This will disconnect any previous connection (if exist) and try to connect again
       * to the network's device selected in the function `connect()`
       *
       * @param timeout             Maximum number of milliseconds that system should wait for
       *                            a successfull connection before is dropped
       * @param connection_intents  Maximum number of intents to connect to server
       *
       * @return 0 if successful, -1 if there is an error (errno is set),
       *         or an error_value is returned
       */
      int reconnect(const int timeout = -1, const int reconnection_intents = 1);
      /**
       * @brief Sends data to a TCP socket stream
       *
       * @param buffer Is a pointer to the data you want to send
       * @param size   Is the number of bytes you want to send
       * @param flags  Allows you to specify more information about how the data is to be sent.
       *          MSG_OOB       Send as “out of band” data. TCP supports this, and it’s a way to
       *                        tell the receiving system that this data has a higher priority
       *                        than the normal data. The receiver will receive the signal SIGURG
       *                        and it can then receive this data without first receiving all
       *                        the rest of the normal data in the queue.
       *          MSG_DONTROUTE Don’t send this data over a router, just keep it local.
       *          MSG_DONTWAIT  If `send()` would block because outbound traffic is clogged, have
       *                        it return `EAGAIN`. This is like a “enable non-blocking just for
       *                        this send.”
       *          MSG_NOSIGNAL  If you `send()` to a remote host which is no longer
       *                        `receive()`ing, you’ll typically get the signal `SIGPIPE`.
       *                        Adding this flag prevents that signal from being raised.
       *
       * @return The number of bytes actually received, or 0 when the server is disconnected,
       *         or -1 on error (and `errno` will be set accordingly).
       */
      ssize_t send(const void *buffer, const std::size_t size, const int flags = MSG_NOSIGNAL);
      /**
       * @brief Sends all required sized data to a TCP socket stream
       *
       * This will loop until all the specified data's size has been sent
       *
       * @param buffer  Is a pointer to the data you want to send
       * @param size    Is the number of bytes you want to send
       * @param flags   Allows you to specify more information about how the data is to be sent.
       *          MSG_OOB       Send as “out of band” data. TCP supports this, and it’s a way to
       *                        tell the receiving system that this data has a higher priority
       *                        than the normal data. The receiver will receive the signal SIGURG
       *                        and it can then receive this data without first receiving all
       *                        the rest of the normal data in the queue.
       *          MSG_DONTROUTE Don’t send this data over a router, just keep it local.
       *          MSG_DONTWAIT  If `send()` would block because outbound traffic is clogged, have
       *                        it return `EAGAIN`. This is like a “enable non-blocking just for
       *                        this send.”
       *          MSG_NOSIGNAL  If you `send()` to a remote host which is no longer
       *                        `receive()`ing, you’ll typically get the signal `SIGPIPE`.
       *                        Adding this flag prevents that signal from being raised.
       *
       * @return The number of bytes actually received, or 0 when the server is disconnected,
       *         or -1 on error (and `errno` will be set accordingly).
       */
      ssize_t send_all(const void *buffer, const std::size_t size,
                       const int flags = MSG_NOSIGNAL);

    private:
      int close();
      int connector(const int timeout, const int connection_intents);

      std::string ip_;
      int port_;
      int socket_fd_;
      bool connected_;
      bool connecting_;
      int socket_type_;
    };

    void signal_children_handler(const int signal);

  } // namespace: network_communication
} // namespace: ramrod

#endif // RAMROD_NETWORK_COMMUNICATION_CLIENT_H
