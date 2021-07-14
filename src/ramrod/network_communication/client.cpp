#include "ramrod/network_communication/client.h"

#include <cerrno>                      // for errno
#include <cstring>                     // for memset
#include <iosfwd>                      // for size_t
#include <netdb.h>                     // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>                    // for sigaction, sigemptyset, SA_RES...
#include <sys/wait.h>                  // for waitpid, WNOHANG
#include <thread>                      // for sleep_for, thread
#include <unistd.h>                    // for ssize_t, close

#include "ramrod/console.h"            // for formatted
#include "ramrod/console/attention.h"  // for attention_stream, attention
#include "ramrod/console/endl.h"       // for endl
#include "ramrod/console/error.h"      // for error, error_stream
#include "ramrod/console/perror.h"     // for perror, perror_stream
#include "ramrod/console/types.h"      // for error
#include "ramrod/console/warning.h"    // for warning, warning_stream

namespace ramrod {
  namespace network_communication {
    client::client() :
      conversor(),
      ip_(),
      port_{1313},
      socket_fd_{-1},
      max_queue_{10},
      current_intent_{0},
      max_intents_{10},
      terminate_concurrent_{false},
      terminate_receive_{false},
      terminate_send_{false},
      connected_{false},
      connecting_{false},
      is_tcp_{false},
      reconnection_time_(std::chrono::milliseconds(5000))
    {}

    client::~client(){
      disconnect();
    }

    bool client::connect(const std::string &ip, const int port, const int socket_type,
                         const bool concurrent){
      if(connecting_.load()) return false;
      if(connected_.load()) disconnect();

      ip_ = ip;
      port_ = port;
      connecting_.store(true);
      current_intent_ = 0;
      is_tcp_ = socket_type != SOCK_DGRAM;

      terminate_concurrent_.store(false);

      if(concurrent)
        std::thread(&client::concurrent_connector, this, true, false).detach();
      else
        concurrent_connector(true, true);
      return true;
    }

    bool client::disconnect(){
      terminate_send_.store(true);
      terminate_receive_.store(true);
      terminate_concurrent_.store(true);

      return close();
    }

    const std::string &client::ip(){
      return ip_;
    }

    bool client::is_connected(){
      return connected_.load(std::memory_order_relaxed);
    }

    int client::max_queue(){
      return max_queue_;
    }

    bool client::max_queue(const int new_max_queue){
      if(new_max_queue <= 0) return false;
      max_queue_ = new_max_queue;
      return true;
    }

    std::uint32_t client::max_reconnection_intents(){
      return max_intents_;
    }

    void client::max_reconnection_intents(const std::uint32_t new_max_intents){
      max_intents_ = new_max_intents;
    }

    int client::port(){
      return port_;
    }

    ssize_t client::receive(char *buffer, const ssize_t size, const int flags){
      if(!connected_.load() || size == 0)
        return 0;

      ssize_t received{::recv(socket_fd_, buffer, static_cast<std::size_t>(size), flags)};
#ifdef VERBOSE
      if(received <= 0) rr::perror("Receiving data");
#endif
      return received;
    }

    ssize_t client::receive_all(char *buffer, const ssize_t size, bool *breaker,
                                const int flags){
      if(!connected_.load() || size == 0)
        return 0;

      ssize_t total_received{0};
      ssize_t bytes_left = size;
      ssize_t received_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_received < size && !(*breaker)){
        if((received_size = ::recv(socket_fd_, buffer + total_received,
                                   static_cast<std::size_t>(bytes_left), flags)) <= 0){
          // The server has disconnected and therefore disconnecting client
          if(received_size == 0)
            return 0;

#ifdef VERBOSE
          rr::perror("Receiving data");
#endif
          // There is an error and returns after the max intents have been reached
          if(++error_counter > max_intents_)
            return -1;

          // This will try to receive the same data than the last time
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        total_received += received_size;
        bytes_left -= received_size;
      }
      return total_received;
    }

    bool client::receive_all_concurrently(char *buffer, ssize_t *size, bool *breaker,
                                          const int flags){
      if(!connected_.load() || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&client::concurrent_receive_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool client::receive_concurrently(char *buffer, ssize_t *size, const int flags){
      if(!connected_.load() || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&client::concurrent_receive, this, buffer, size, flags).detach();
      return true;
    }

    bool client::reconnect(const bool concurrent){
      if(ip_.size() == 0 || port_ <= 0) return false;
      if(connecting_.load()) return true;
      if(connected_.load()) disconnect();

      connecting_.store(true);
      current_intent_ = 0;

      if(concurrent)
        std::thread(&client::concurrent_connector, this, true, false).detach();
      else
        concurrent_connector(true, true);
      return true;
    }

    ssize_t client::send(const char *buffer, const ssize_t size, const int flags){
      if(!connected_.load() || size == 0)
        return 0;

      ssize_t sent{::send(socket_fd_, buffer, static_cast<std::size_t>(size), flags)};
#ifdef VERBOSE
      if(sent <= 0) rr::perror("Sending data");
#endif
      return sent;
    }

    ssize_t client::send_all(const char *buffer, const ssize_t size, bool *breaker,
                             const int flags){
      if(!connected_.load() || size == 0)
        return 0;

      ssize_t total_sent{0};
      ssize_t bytes_left = size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < size && !(*breaker)){
        if((sent_size = ::send(socket_fd_, buffer + total_sent,
                               static_cast<std::size_t>(bytes_left), flags)) <= 0){
          // The server has disconnected and therefore disconnecting client
          if(sent_size == 0)
            return 0;

#ifdef VERBOSE
          rr::perror("Sending data");
#endif
          // There is an error and returns after the max intents have been reached
          if(++error_counter > max_intents_)
            return -1;

          // This will try to send the same data than the last time
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        total_sent += sent_size;
        bytes_left -= sent_size;
      }
      return total_sent;
    }

    bool client::send_all_concurrently(const char *buffer, ssize_t *size, bool *breaker,
                                       const int flags){
      if(!connected_.load() || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&client::concurrent_send_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool client::send_concurrently(const char *buffer, ssize_t *size, const int flags){
      if(!connected_.load() || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&client::concurrent_send, this, buffer, size, flags).detach();
      return true;
    }

    int client::time_to_reconnect(){
      return static_cast<int>(reconnection_time_.count());
    }

    void client::time_to_reconnect(const int waiting_time_in_milliseconds){
      if(waiting_time_in_milliseconds > 0)
        reconnection_time_ = std::chrono::duration<long, std::milli>(
              std::chrono::milliseconds(waiting_time_in_milliseconds)
            );
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    bool client::close(){
      if(socket_fd_ < 0){
#ifdef VERBOSE
        rr::warning("Connection already closed.");
#endif
        return true;
      }

      if(::shutdown(socket_fd_, SHUT_RDWR) == -1)
        rr::perror("Connection cannot be shutdown");

      if(::close(socket_fd_) == -1){
        rr::perror("Connection cannot be closed");
        return false;
      }

      socket_fd_ = -1;
      connected_.store(false);
      return true;
    }

    void client::concurrent_connector(const bool force, const bool wait){
      if(!force && terminate_concurrent_.load()){
        terminate_concurrent_.store(false);
        return;
      }
      if(connected_.load()) return;

      int status;
      struct addrinfo hints;
      struct addrinfo *results; // Will point to the results
      struct addrinfo *pointer;

      std::memset(&hints, 0, sizeof(addrinfo));               // make sure the struct is empty
      hints.ai_family   = AF_UNSPEC;                          // don't care if IPv4 or IPv6
      hints.ai_socktype = is_tcp_ ? SOCK_STREAM : SOCK_DGRAM; // UDP or TCP socket
      hints.ai_flags    = AI_PASSIVE;                         // Fill in my Ip information for me

      // first, load up address structs with getaddrinfo():
      if((status = ::getaddrinfo(ip_.c_str(), std::to_string(port_).c_str(),
                                 &hints, &results)) != 0){
        rr::formatted("Error: getaddrinfo (%s)\n", rr::message::error, ::gai_strerror(status));
        return;
      }

      // loop through all the results and bind to the first we can
      for(pointer = results; pointer != nullptr; pointer = pointer->ai_next){
        // Making a socket
        if((socket_fd_ = ::socket(pointer->ai_family, pointer->ai_socktype,
                                  pointer->ai_protocol)) == -1){
          rr::perror("Selecting socket");
          continue;
        }

        // Connecting to the server
        if(::connect(socket_fd_, pointer->ai_addr, pointer->ai_addrlen) == -1){
          rr::perror("Connecting");
          ::close(socket_fd_);
          continue;
        }
        break;
      }

      ::freeaddrinfo(results); // all done with this structure

      if(pointer == nullptr){
        rr::perror("client failed to bind");
        if(++current_intent_ > max_intents_){
          rr::error("Max number of reconnections has been reached "
                    "and therefore failed to connect.");
          return;
        }
        rr::attention() << "Trying reconnection in " << reconnection_time_.count() / 1000
                        << " seconds... (#" << current_intent_ << ")" << rr::endl;
        std::this_thread::sleep_for(reconnection_time_);

        // Terminates the pending connection in case disconnect() is called:
        if(terminate_concurrent_.load()) return;

        rr::attention("Reconnecting!");

        if(wait)
          concurrent_connector(false, true);
        else
          std::thread(&client::concurrent_connector, this, false, false).detach();

        return;
      }

      // TODO: is this necessary?
      struct sigaction signal_action;

      // Reaping all dead processes
      signal_action.sa_handler = signal_children_handler;
      ::sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = SA_RESTART;
      if(::sigaction(SIGCHLD, &signal_action, nullptr) == -1){
        rr::perror("Reaping dead processes");
        return;
      }
      // END TODO:

      char outgoing[11] = "identifier";
      if(::send(socket_fd_, outgoing, sizeof(outgoing), MSG_NOSIGNAL) < 0){
#ifdef VERBOSE
        rr::perror("Sending identifier");
#endif
      }

      connected_.store(true);
      connecting_.store(false);
      terminate_receive_.store(false);
      terminate_send_.store(false);
#ifdef VERBOSE
      rr::attention("Connected to server!");
#endif
    }

    void client::concurrent_receive(char *buffer, ssize_t *size, const int flags){
      ssize_t received{::recv(socket_fd_, buffer, static_cast<std::size_t>(*size), flags)};
#ifdef VERBOSE
      if(received <= 0) rr::perror("Receiving data");
#endif
      *size = received;
    }

    void client::concurrent_receive_all(char *buffer, ssize_t *size, bool *breaker,
                                        const int flags){
      ssize_t total_received{0};
      ssize_t bytes_left = *size;
      ssize_t received_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_received < *size && !terminate_receive_.load() && !(*breaker)){
        if((received_size = ::recv(socket_fd_, buffer + total_received,
                                   static_cast<std::size_t>(bytes_left), flags)) <= 0){
          // The server has disconnected and therefore disconnecting client
          if(received_size == 0){
            *size = 0;
            return;
          }
#ifdef VERBOSE
          rr::perror("Receiving data");
#endif
          // There is an error and returns after the max intents have been reached
          if(++error_counter > max_intents_){
            *size = -1;
            return;
          }

          // This will try to receive the same data than the last time
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        total_received += received_size;
        bytes_left -= received_size;
      }
      *size = total_received;
    }

    void client::concurrent_send(const char *buffer, ssize_t *size, const int flags){
      ssize_t sent{::send(socket_fd_, buffer, static_cast<std::size_t>(*size), flags)};
#ifdef VERBOSE
      if(sent <= 0) rr::perror("Sending data");
#endif
      *size = sent;
    }

    void client::concurrent_send_all(const char *buffer, ssize_t *size, bool *breaker,
                                     const int flags){
      ssize_t total_sent{0};
      ssize_t bytes_left = *size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < *size && !terminate_send_.load() && !(*breaker)){
        if((sent_size = ::send(socket_fd_, buffer + total_sent,
                               static_cast<std::size_t>(bytes_left), flags)) <= 0){
          if(sent_size == 0){
            *size = 0;
            return;
          }
#ifdef VERBOSE
          rr::perror("Sending data");
#endif
          // There is an error and returns after the max intents have been reached
          if(++error_counter > max_intents_){
            *size = -1;
            return;
          }

          // This will try to receive the same data than the last time
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          continue;
        }
        total_sent += sent_size;
        bytes_left -= sent_size;
      }
      *size = total_sent;
    }

    // :::::::::::::::::::::::::::::::::::: OUTTER FUNCTIONS :::::::::::::::::::::::::::::::::::

    void signal_children_handler(const int /*signal*/){
      // waitpid() might overwrite errno, so we save and restore it:
      const int saved_errno = errno;
      while(::waitpid(-1, nullptr, WNOHANG) > 0);
      errno = saved_errno;
    }
  } // namespace: network_communication
} // namespace: ramrod
