#include "ramrod/network_communication/server.h"

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
    server::server() :
      conversor(),
      ip_(),
      port_{1313},
      socket_fd_{-1},
      connected_fd_{-1},
      max_queue_{10},
      current_intent_{0},
      max_intents_{10},
      terminate_concurrent_{false},
      terminate_receive_{false},
      terminate_send_{false},
      connected_{false},
      connecting_{false},
      is_tcp_{false},
      client_{nullptr},
      results_{nullptr},
      reconnection_time_(std::chrono::milliseconds(5000))
    {}

    server::~server(){
      disconnect();
    }

    bool server::connect(const std::string ip, const int port, const int socket_type){
      if(connecting_) return false;
      if(connected_) disconnect();

      ip_ = ip;
      port_ = port;
      connecting_ = true;
      current_intent_ = 0;
      is_tcp_ = socket_type != SOCK_DGRAM;

      terminate_concurrent_ = false;

      std::thread(&server::concurrent_connector, this, true).detach();
      return true;
    }

    bool server::disconnect(){
      terminate_send_ = true;
      terminate_receive_ = true;
      terminate_concurrent_ = true;

      if(results_) ::freeaddrinfo(results_);

      return close_child() & close();
    }

    const std::string &server::ip(){
      return ip_;
    }

    bool server::is_connected(){
      return connected_;
    }

    int server::max_queue(){
      return max_queue_;
    }

    bool server::max_queue(const int new_max_queue){
      if(new_max_queue <= 0) return false;
      max_queue_ = new_max_queue;
      return true;
    }

    std::uint32_t server::max_reconnection_intents(){
      return max_intents_;
    }

    void server::max_reconnection_intents(const std::uint32_t new_max_intents){
      max_intents_ = new_max_intents;
    }

    int server::port(){
      return port_;
    }

    ssize_t server::receive(char *buffer, const ssize_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      ssize_t received{0};
      if(is_tcp_)
        received = ::recv(connected_fd_, buffer, static_cast<std::size_t>(size), flags);
      else
        received = ::recvfrom(socket_fd_, buffer, static_cast<std::size_t>(size), flags,
                              client_->ai_addr, &client_->ai_addrlen);
#ifdef VERBOSE
      if(received < 0) rr::perror("Receiving data " + std::to_string(socket_fd_) + " : " + std::to_string(connected_fd_));
#endif
      if(received == 0) disconnect();
      return received;
    }

    ssize_t server::receive_all(char *buffer, const ssize_t size, bool *breaker,
                                const int flags){
      if(!connected_ || size == 0)
        return 0;

      ssize_t total_received{0};
      ssize_t bytes_left = size;
      ssize_t received_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_received < size && !(*breaker)){
        if(is_tcp_)
          received_size = ::recv(connected_fd_, buffer + total_received,
                                 static_cast<std::size_t>(bytes_left), flags);
        else
          received_size = ::recvfrom(socket_fd_, buffer + total_received,
                                     static_cast<std::size_t>(bytes_left), flags,
                                     client_->ai_addr, &client_->ai_addrlen);
        // The client has disconnected and therefore disconnecting server
        if(received_size == 0){
          disconnect();
          return 0;
        }

        if(received_size < 0){
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

    bool server::receive_all_concurrently(char *buffer, ssize_t *size, bool *breaker,
                                          const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_receive_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool server::receive_concurrently(char *buffer, ssize_t *size, const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_receive, this, buffer, size, flags).detach();
      return true;
    }

    bool server::reconnect(){
      if(connecting_ || ip_.size() == 0 || port_ <= 0) return false;

      if(connected_) disconnect();

      connecting_ = true;
      current_intent_ = 0;

      std::thread(&server::concurrent_connector, this, true).detach();
      return true;
    }

    ssize_t server::send(const char *buffer, const ssize_t size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      ssize_t sent{0};
      if(is_tcp_)
        sent = ::send(connected_fd_, buffer, static_cast<std::size_t>(size), flags);
      else
        sent = ::sendto(socket_fd_, buffer, static_cast<std::size_t>(size), flags,
                        client_->ai_addr, client_->ai_addrlen);
#ifdef VERBOSE
      if(sent < 0) rr::perror("Sending data");
#endif
      if(sent == 0) disconnect();
      return sent;
    }

    ssize_t server::send_all(const char *buffer, const ssize_t size, bool *breaker,
                             const int flags){
      if(!connected_ || size == 0)
        return 0;

      ssize_t total_sent{0};
      ssize_t bytes_left = size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < size && !(*breaker)){
        if(is_tcp_)
          sent_size = ::send(connected_fd_, buffer + total_sent, 
                             static_cast<std::size_t>(bytes_left), flags);
        else
          sent_size = ::sendto(socket_fd_, buffer + total_sent, 
                               static_cast<std::size_t>(bytes_left), flags,
                               client_->ai_addr, client_->ai_addrlen);

        // The client has disconnected and therefore disconnecting server
        if(sent_size == 0){
          disconnect();
          return 0;
        }

        if(sent_size < 0){
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

    bool server::send_all_concurrently(const char *buffer, ssize_t *size, bool *breaker,
                                       const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_send_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool server::send_concurrently(const char *buffer, ssize_t *size, const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_send, this, buffer, size, flags).detach();
      return true;
    }

    int server::time_to_reconnect(){
      return static_cast<int>(reconnection_time_.count());
    }

    void server::time_to_reconnect(const int waiting_time_in_milliseconds){
      if(waiting_time_in_milliseconds > 0)
        reconnection_time_ = std::chrono::duration<long, std::milli>(
              std::chrono::milliseconds(waiting_time_in_milliseconds)
            );
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    bool server::close(){
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
      return true;
    }

    bool server::close_child(){
      if(connected_fd_ < 0){
#ifdef VERBOSE
        rr::warning("Children connection already closed.");
#endif
        return !(connected_ = false);
      }

      if(::shutdown(connected_fd_, SHUT_RDWR) == -1)
        rr::perror("Children connection cannot be shutdown");

      if(::close(connected_fd_) == -1){
        rr::perror("Children connection cannot be closed");
        return false;
      }

      connected_fd_ = -1;
      return !(connected_ = false);
    }

    void server::concurrent_connector(const bool force){
      if(!force && terminate_concurrent_){
        terminate_concurrent_ = false;
        return;
      }
      if(connected_) return;

      int status;
      struct addrinfo hints;

      std::memset(&hints, 0, sizeof(addrinfo));               // make sure the struct is empty
      hints.ai_family   = AF_UNSPEC;                          // don't care if IPv4 or IPv6
      hints.ai_socktype = is_tcp_ ? SOCK_STREAM : SOCK_DGRAM; // UDP or TCP socket
      hints.ai_flags    = AI_PASSIVE;                         // Fill in my Ip information for me

      // first, load up address structs with getaddrinfo():
      if((status = ::getaddrinfo(ip_.c_str(), std::to_string(port_).c_str(),
                                 &hints, &results_)) != 0){
        rr::formatted("Error: getaddrinfo (%s)\n", rr::message::error, ::gai_strerror(status));
        return;
      }

      // loop through all the results and bind to the first we can
      for(client_ = results_; client_ != nullptr; client_ = client_->ai_next){
        // Making a socket
        if((socket_fd_ = ::socket(client_->ai_family, client_->ai_socktype,
                                  client_->ai_protocol)) == -1){
          rr::perror("Selecting socket");
          continue;
        }

        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(int)) == -1){
          rr::perror("Setting socket options");
          ::close(socket_fd_);
          return;
        }

        // Binding the socket to the port
        if(::bind(socket_fd_, client_->ai_addr, client_->ai_addrlen) == -1){
          rr::perror("Binding socket");
          ::close(socket_fd_);
          continue;
        }
        break;
      }

      if(is_tcp_) ::freeaddrinfo(results_); // all done with this structure

      if(client_ == nullptr){
        if(results_) freeaddrinfo(results_);
        results_ = nullptr;

        rr::perror("Server failed to bind");
        if(++current_intent_ > max_intents_){
          rr::error("Max number of reconnections has been reached "
                    "and therefore failed to connect.");
          return;
        }
        rr::attention() << "Trying reconnection in " << reconnection_time_.count() / 1000
                        << " seconds... (#" << current_intent_ << ")" << rr::endl;
        std::this_thread::sleep_for(reconnection_time_);

        // Terminates the pending connection in case disconnect() is called:
        if(terminate_concurrent_) return;

        rr::attention("Reconnecting!");
        std::thread(&server::concurrent_connector, this, false).detach();
        return;
      }

      if(is_tcp_)
        if(::listen(socket_fd_, max_queue_) == -1){
          rr::perror("Listening to socket");
          return;
        }

      struct sigaction signal_action;

      // Reaping all dead processes
      signal_action.sa_handler = signal_children_handler;
      ::sigemptyset(&signal_action.sa_mask);
      signal_action.sa_flags = SA_RESTART;
      if(::sigaction(SIGCHLD, &signal_action, nullptr) == -1){
        rr::perror("Reaping dead processes");
        return;
      }

      // If is UDP we terminate sooner so we do not accept() any incomming connection
      if(!is_tcp_){
        connected_ = true;
        connecting_ = false;
        terminate_receive_ = false;
        terminate_send_ = false;
        return;
      }
      // In case is TCP then we must accept an incomming connection
      std::thread(&server::concurrent_connection, this).detach();
    }

    void server::concurrent_connection(){
#ifdef VERBOSE
      rr::attention("Waiting for incomming connections!");
#endif
      struct sockaddr_storage their_addr;
      socklen_t addr_size;

      while(!terminate_concurrent_){
        addr_size = sizeof(their_addr);
        // Accept incoming connection
        if((connected_fd_ = ::accept(socket_fd_, (struct sockaddr*)&their_addr, &addr_size)) == -1){
          rr::perror("Accepting connection");
          continue;
        }
#ifdef VERBOSE
        rr::attention("Connection established!");
#endif
        connected_ = true;
        connecting_ = false;
        terminate_receive_ = false;
        terminate_send_ = false;

        break;
      }
    }

    void server::concurrent_receive(char *buffer, ssize_t *size, const int flags){
      ssize_t received{0};
      if(is_tcp_)
        received = ::recv(connected_fd_, buffer, static_cast<std::size_t>(*size), flags);
      else
        received = ::recvfrom(socket_fd_, buffer, static_cast<std::size_t>(*size), flags,
                              client_->ai_addr, &client_->ai_addrlen);
#ifdef VERBOSE
      if(received < 0) rr::perror("Receiving data");
#endif
      if(received == 0) disconnect();
      *size = received;
    }

    void server::concurrent_receive_all(char *buffer, ssize_t *size, bool *breaker,
                                        const int flags){
      ssize_t total_received{0};
      ssize_t bytes_left = *size;
      ssize_t received_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_received < *size && !terminate_send_ && !(*breaker)){
        if(is_tcp_)
          received_size = ::recv(connected_fd_, buffer + total_received,
                                 static_cast<std::size_t>(bytes_left), flags);
        else
          received_size = ::recvfrom(socket_fd_, buffer + total_received,
                                     static_cast<std::size_t>(bytes_left), flags,
                                     client_->ai_addr, &client_->ai_addrlen);
        // The client has disconnected and therefore disconnecting server
        if(received_size == 0){
          *size = 0;
          disconnect();
          return;
        }

        if(received_size < 0){
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

    void server::concurrent_send(const char *buffer, ssize_t *size, const int flags){
      ssize_t sent{0};
      if(is_tcp_)
        sent = ::send(connected_fd_, buffer, static_cast<std::size_t>(*size), flags);
      else
        sent = ::sendto(socket_fd_, buffer, static_cast<std::size_t>(*size), flags,
                        client_->ai_addr, client_->ai_addrlen);
#ifdef VERBOSE
      if(sent < 0) rr::perror("Sending data");
#endif
      if(sent == 0) disconnect();
      *size = sent;
    }

    void server::concurrent_send_all(const char *buffer, ssize_t *size, bool *breaker,
                                     const int flags){
      ssize_t total_sent{0};
      ssize_t bytes_left = *size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < *size && !terminate_send_ && !(*breaker)){
        if(is_tcp_)
          sent_size = ::send(connected_fd_, buffer + total_sent, 
                             static_cast<std::size_t>(bytes_left), flags);
        else
          sent_size = ::sendto(socket_fd_, buffer + total_sent, 
                               static_cast<std::size_t>(bytes_left), flags,
                               client_->ai_addr, client_->ai_addrlen);
        // The client has disconnected and therefore disconnecting server
        if(sent_size == 0){
          *size = 0;
          disconnect();
          return;
        }

        if(sent_size < 0){
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
