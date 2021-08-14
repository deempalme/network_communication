#ifndef RAMROD_NETWORK_COMMUNICATION_CLIENT_H
#define RAMROD_NETWORK_COMMUNICATION_CLIENT_H

#include <atomic>        // for atomic
#include <cstdint>       // for uint32_t, uint16_t
#include <sys/types.h>   // for ssize_t
#include <sys/socket.h>  // for recv, send, MSG_NOSIGNAL, accept
#include <chrono>        // for duration
#include <iosfwd>        // for size_t
#include <ratio>         // for milli
#include <string>        // for string

#include "ramrod/network_communication/conversor.h"  // for conversor


namespace ramrod {
  namespace network_communication {
    class client : public conversor
    {
    public:
      client();
      ~client();
      /**
       * @brief Makes a TCP socket stream connection to an specific IP and port
       *
       * This will disconnect any previous connection and it will create a concurrent
       * one to another network's device, so the main thread will be not interrupted
       * because is waiting for the device with our selected IP address to connect.
       *
       * @param ip          Selected IP address to connect
       * @param port        Port number to where the connection will be made
       * @param socket_type Defines the type of connection TCP or UDP, options:
       *                      SOCK_STREAM   Creates a TCP socket
       *                      SOCK_DGRAM    Creates a connected datagram socket
       * @param concurrent  Indicates if the connection should be made in a different
       *                    thread, in this way the main thread should not await for the
       *                    server to connect with us
       *
       * @return `false` if there is already a pending connection open, call `disconnect()`
       *         to cancel such connection
       */
      bool connect(const std::string &ip, const int port = 1313,
                   const int socket_type = SOCK_STREAM, const bool concurrent = true);
      /**
       * @brief Disconnects this device from the current connected network's device
       *
       * @return `false` if the connection cannot be closed
       */
      bool disconnect();
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
       * @brief Getting how many pending connections you can have before the
       *        kernel starts rejecting new ones.
       *
       * @return Maximum number of pending connections, default is 10
       */
      int max_queue();
      /**
       * @brief Setting how many pending connections you can have before the
       *        kernel starts rejecting new ones.
       *
       * @param new_max_queue New value for maximum pending connections
       *
       * @return `false` if the value is negative
       */
      bool max_queue(const int new_max_queue);
      /**
       * @brief Getting the maximum number of intents to connect to another network's device
       *
       * @return Number of maximum reconnection intents, default is 10
       */
      std::uint32_t max_reconnection_intents();
      /**
       * @brief Setting the maximum number of intents to connect to another network's device
       *
       * @param new_max_intents New number of maximum reconnection intents
       */
      void max_reconnection_intents(const std::uint32_t new_max_intents);
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
       * @param breaker Is a pointer to a boolean variable that will break the infinite loop
       *                when is set to `true`, if left as `nullptr` then it will keep waiting for
       *                data until the size is fullfilled
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
      ssize_t receive_all(void *buffer, const std::size_t size, bool *breaker = nullptr,
                          const int flags = 0);
      /**
       * @brief Receives all required sized data from a TCP socket stream in a different thread
       *
       * This will loop until all the specified data's size has been received
       *
       * @param buffer  Is a pointer to the data you want to receive
       * @param size    Is a pointer to the number of bytes you want to receive, when the task is
       *                finished it will return the total number of bytes actually received, or 0
       *                when the server is disconnected, or on error (and `errno` will be set
       *                accordingly).
       * @param breaker Is a pointer to a boolean variable that will break the infinite loop
       *                when is set to `true`, if left as `nullptr` then it will keep waiting for
       *                data until the size is fullfilled
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
       * @return `false` if there is no open connection.
       */
      bool receive_all_concurrently(void *buffer, std::size_t *size, bool *breaker = nullptr,
                                    const int flags = 0);
      /**
       * @brief Receives data from a TCP socket stream in a different thread
       *
       * @param buffer Is a pointer to the data you want to receive
       * @param size   Is a pointer to the number of bytes you want to receive, when the task is
       *               finished it will return the total number of bytes actually received, or 0 
       *               when the server is disconnected, or on error (and `errno` will be set
       *               accordingly).
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
       * @return `false` if there is no open connection.
       */
      bool receive_concurrently(void *buffer, std::size_t *size, const int flags = 0);
      /**
       * @brief Reconnecting again
       *
       * This will disconnect any previous connection (if exist) and try to connect again
       * to the network's device selected in the function `connect()`, and, as in `connect()`
       * it will also be performed in a different thread.
       *
       * @param concurrent Indicates if the reconnection should be made in a different thread,
       *                   in this way the main thread should not await for the server to
       *                   connect with us
       *
       * @return `false` if there is no IP or port selected, it will return `true` if
       *          there is an open pending connection, or already waiting for connection
       */
      bool reconnect(const bool concurrent = true);
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
       * @param breaker Is a pointer to a boolean variable that will break the infinite loop
       *                when is set to `true`, if left as `nullptr` then it will keep waiting for
       *                data until the size is fullfilled
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
      ssize_t send_all(const void *buffer, const std::size_t size, bool *breaker = nullptr,
                       const int flags = MSG_NOSIGNAL);
      /**
       * @brief Sends all required sized data to a TCP socket stream in a different thread
       *
       * This will loop until all the specified data's size has been sent
       *
       * @param buffer Is a pointer to the data you want to send
       * @param size   Is a pointer to the number of bytes you want to send, when the task is
       *               finished it will return the total number of bytes actually sent, or 0
       *               when the server is disconnected, or on error (and `errno` will be set
       *               accordingly).
       * @param breaker Is a pointer to a boolean variable that will break the infinite loop
       *                when is set to `true`, if left as `nullptr` then it will keep waiting for
       *                data until the size is fullfilled
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
       * @return `false` if there is no open connection.
       */
      bool send_all_concurrently(const void *buffer, std::size_t *size, bool *breaker = nullptr,
                                 const int flags = MSG_NOSIGNAL);
      /**
       * @brief Sends data to a TCP socket stream in a different thread
       *
       * @param buffer Is a pointer to the data you want to send
       * @param size   Is a pointer to the number of bytes you want to send, when the task is
       *               finished it will return the total number of bytes actually sent, or 0 
       *               when the server is disconnected, or on error (and `errno` will be set
       *               accordingly).
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
       * @return `false` if there is no open connection.
       */
      bool send_concurrently(const void *buffer, std::size_t *size, const int flags = MSG_NOSIGNAL);
      /**
       * @brief Gettting the current time that this device will wait to try to connect
       *        again if the previous intent to establish a connection failed
       *
       * @return Current time that this device awaits
       */
      int time_to_reconnect();
      /**
       * @brief Settting the current time that this device will wait to try to connect
       *        again if the previous intent to establish a connection failed
       *
       * @param waiting_time_in_milliseconds New Current time that this device will wait
       */
      void time_to_reconnect(const int waiting_time_in_milliseconds);

    private:
      bool close();

      void concurrent_connector(const bool wait = false);

      void concurrent_receive(void *buffer, std::size_t *size, const int flags);
      void concurrent_receive_all(void *buffer, std::size_t *size, bool *breaker, const int flags);
      void concurrent_send(const void *buffer, std::size_t *size, const int flags);
      void concurrent_send_all(const void *buffer, std::size_t *size, bool *breaker, const int flags);

      std::string ip_;
      int port_;
      int socket_fd_;
      int max_queue_;
      std::uint32_t current_intent_;
      std::uint32_t max_intents_;
      std::atomic<bool> terminate_concurrent_;
      std::atomic<bool> terminate_receive_;
      std::atomic<bool> terminate_send_;
      std::atomic<bool> connected_;
      std::atomic<bool> connecting_;
      bool is_tcp_;
      std::chrono::duration<long, std::milli> reconnection_time_;
    };

    void signal_children_handler(const int signal);

  } // namespace: network_communication
} // namespace: ramrod

#endif // RAMROD_NETWORK_COMMUNICATION_CLIENT_H
