#include "ramrod/network_communication/client.h"

#include <cerrno>       // for errno
#include <cstring>      // for memset
#include <iosfwd>       // for size_t
#include <cmath>        // for floor
#include <netdb.h>      // for addrinfo, freeaddrinfo, gai_st...
#include <signal.h>     // for sigaction, sigemptyset, SA_RES...
#include <sys/types.h>  // for ssize_t
#include <sys/wait.h>   // for waitpid, WNOHANG
#include <unistd.h>     // for ssize_t, close


namespace ramrod {
  namespace network_communication {
    client::client() :
      conversor(),
      ip_{},
      port_{1313},
      socket_fd_{-1},
      connected_{false},
      connecting_{false},
      socket_type_{0}
    {}

    client::~client(){
      disconnect();
    }

    int client::connect(const std::string &ip, const int port, const int socket_type,
                        const int timeout, const int connection_intents){
      if(connected_) disconnect();

      ip_ = ip;
      port_ = port;
      socket_type_ = socket_type;

      return connector(timeout, connection_intents);
    }

    bool client::connecting(){
      return connecting_;
    }

    int client::disconnect(){
      return close();
    }

    const std::string &client::ip(){
      return ip_;
    }

    bool client::is_connected(){
      return connected_;
    }

    int client::port(){
      return port_;
    }

    ssize_t client::receive(void *buffer, const std::size_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      return ::recv(socket_fd_, buffer, size, flags);
    }

    ssize_t client::receive_all(void *buffer, const std::size_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      std::size_t total_received{0};
      std::size_t bytes_left{size};
      ssize_t received_size;

      while(total_received < size){
        received_size = ::recv(socket_fd_, static_cast<std::uint8_t*>(buffer) + total_received,
                               bytes_left, flags);

        if(received_size == 0)
          return 0;
        else if(received_size < 0)
          return received_size;

        total_received += static_cast<std::size_t>(received_size);
        bytes_left -= static_cast<std::size_t>(received_size);
      }
      return static_cast<ssize_t>(total_received);
    }

    int client::reconnect(const int timeout, const int reconnection_intents){
      if(ip_.size() == 0 || port_ <= 0) return error_value::port_or_ip_not_defined;
      if(connecting_) return error_value::already_connecting;
      if(connected_) disconnect();

      connecting_ = true;

      return connector(timeout, reconnection_intents);
    }

    ssize_t client::send(const void *buffer, const std::size_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      return ::send(socket_fd_, buffer, size, flags);
    }

    ssize_t client::send_all(const void *buffer, const std::size_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      std::size_t total_sent{0};
      std::size_t bytes_left = size;
      ssize_t sent_size;

      while(total_sent < size){
        sent_size = ::send(socket_fd_, (const std::uint8_t*)buffer + total_sent,
                           bytes_left, flags);
        // The server has disconnected and therefore disconnecting client
        if(sent_size == 0)
          return 0;
        else if(sent_size < 0)
          return sent_size;

        total_sent += static_cast<std::size_t>(sent_size);
        bytes_left -= static_cast<std::size_t>(sent_size);
      }
      return static_cast<ssize_t>(total_sent);
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    int client::close(){
      connecting_ = false;

      if(socket_fd_ < 0)
        return error_value::no_error;

      if(::shutdown(socket_fd_, SHUT_RDWR) == -1)
        return errno;

      if(::close(socket_fd_) == -1)
        return errno;

      socket_fd_ = -1;
      connected_ = false;
      return error_value::no_error;
    }

    int client::connector(const int timeout, const int connection_intents){
      if(connected_) return error_value::no_error;
      if(connection_intents <= 0){
        connecting_ = false;
        return error_value::reached_max_intents;
      }

      connecting_ = true;
      struct addrinfo hints;
      struct addrinfo *results; // Will point to the results
      struct addrinfo *pointer;

      std::memset(&hints, 0, sizeof(addrinfo)); // make sure the struct is empty
      hints.ai_family   = AF_UNSPEC;            // don't care if IPv4 or IPv6
      hints.ai_socktype = socket_type_;         // UDP or TCP socket
      hints.ai_flags    = AI_PASSIVE;           // Fill in my Ip information for me

      int status;

      struct timeval initial_waiting;
      socklen_t waiting_length{sizeof(initial_waiting)};

      // first, load up address structs with getaddrinfo():
      if((status = ::getaddrinfo(ip_.c_str(), std::to_string(port_).c_str(),
                                 &hints, &results)) != 0){
        errno = status;
        connecting_ = false;
        return error_value::getaddrinfo_error;
      }

      // loop through all the results and bind to the first we can
      for(pointer = results; pointer != nullptr; pointer = pointer->ai_next){
        // Making a socket
        if((socket_fd_ = ::socket(pointer->ai_family, pointer->ai_socktype,
                                  pointer->ai_protocol)) == -1){
          continue;
        }

        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(int)) == -1){
          ::close(socket_fd_);
          ::freeaddrinfo(results);
          goto error_reached;
        }

        // Setting the connection timeout
        if(timeout > 0){
          if(::getsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO,
                          &initial_waiting, &waiting_length) == -1){
            ::close(socket_fd_);
            ::freeaddrinfo(results);
            goto error_reached;
          }

          const double to_seconds{static_cast<double>(timeout)/1000.0};
          struct timeval waiting;
          waiting.tv_sec  = static_cast<__time_t>(std::floor(to_seconds));
          waiting.tv_usec = static_cast<__suseconds_t>((to_seconds - std::floor(to_seconds))
                                                       * 1000.0f) * 1000;
          if(::setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &waiting, sizeof(waiting)) == -1){
            ::close(socket_fd_);
            ::freeaddrinfo(results);
            goto error_reached;
          }
        }

        // Connecting to the server
        if(::connect(socket_fd_, pointer->ai_addr, pointer->ai_addrlen) == -1){
          ::close(socket_fd_);
          continue;
        }
        break;
      }

      ::freeaddrinfo(results); // all done with this structure

      if(timeout > 0){
        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &initial_waiting, waiting_length) == -1){
          ::close(socket_fd_);
          goto error_reached;
        }
      }

      if(pointer == nullptr)
        return connector(timeout, connection_intents - 1);

      // TODO: is this necessary?
      struct sigaction signal_action;

      // Reaping all dead processes
      signal_action.sa_handler = signal_children_handler;
      ::sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = SA_RESTART;
      if(::sigaction(SIGCHLD, &signal_action, nullptr) == -1)
        goto error_reached;
      // END TODO

      connected_ = true;
      connecting_ = false;
      return error_value::no_error;

      error_reached:

      connected_ = false;
      connecting_ = false;
      return errno;
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
