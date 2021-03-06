#include "ramrod/network_communication/server.h"

#include <cerrno>                      // for errno
#include <cstring>                     // for memset
#include <iosfwd>                      // for size_t
#include <netdb.h>                     // for addrinfo, freeaddrinfo, gai_st...
#include <netinet/in.h>                // for htonl, htons, ntohl, ntohs
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
      reconnection_time_(std::chrono::milliseconds(5000))
    {}

    server::~server(){
      disconnect();
    }

    bool server::connect(const std::string ip, const int port){
      // TODO: cancell all pending connections?
      if(connecting_) return false;
      if(connected_) disconnect();

      ip_ = ip;
      port_ = port;
      connecting_ = true;
      current_intent_ = 0;

      std::thread(&server::concurrent_connector, this, true).detach();
      return true;
    }

    bool server::disconnect(){
      terminate_send_ = true;
      terminate_receive_ = true;
      terminate_concurrent_ = true;

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
      if(!connected_){
        rr::error("There is no connection to receive from.");
        return -1;
      }

      ssize_t total_received{0};
      ssize_t bytes_left = size;
      ssize_t received_size;
      std::uint32_t error_counter{0};

      while(total_received < size){
        if((received_size = ::recv(connected_fd_, buffer + total_received,
                                   static_cast<std::size_t>(bytes_left), flags)) == -1){
          rr::perror("Receiving data");
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          if(++error_counter > max_intents_){
            total_received = -1;
            break;
          }
        }
        total_received += received_size;
        bytes_left -= received_size;
      }
      return total_received;
    }

    bool server::receive_concurrently(char *buffer, ssize_t *size, const int flags){
      if(!connected_){
        rr::error("There is no connection to receive from.");
        *size = -1;
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
      if(!connected_){
        rr::error("There is no connection to send from.");
        return -1;
      }

      ssize_t total_sent{0};
      ssize_t bytes_left = size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};

      while(total_sent < size){
        if((sent_size = ::send(connected_fd_, buffer + total_sent,
                               static_cast<std::size_t>(bytes_left), flags)) == -1){
          rr::perror("Sending data");
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          if(++error_counter > max_intents_){
            total_sent = -1;
            break;
          }
        }
        total_sent += sent_size;
        bytes_left -= sent_size;
      }
      return total_sent;
    }

    bool server::send_concurrently(const char *buffer, ssize_t *size, const int flags){
      if(!connected_){
        rr::error("There is no connection to send from.");
        *size = -1;
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
        rr::warning("Connection already closed.");
        return true;
      }

      if(::close(socket_fd_) == -1){
        rr::perror("Connection cannot be closed");
        return false;
      }

      socket_fd_ = -1;
      return true;
    }

    bool server::close_child(){
      if(connected_fd_ < 0){
        rr::warning("Children connection already closed.");
        return !(connected_ = false);
      }

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
      struct addrinfo *results; // Will point to the results
      struct addrinfo *pointer;

      std::memset(&hints, 0, sizeof(hints));  // make sure the struct is empty
      hints.ai_family = AF_UNSPEC;            // don't care if IPv4 or IPv6
      hints.ai_socktype = SOCK_STREAM;        // TCP stream sockets
      hints.ai_flags = AI_PASSIVE;            // Fill in my Ip information for me

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

        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(int)) == -1){
          rr::perror("Setting socket options");
          ::close(socket_fd_);
          return;
        }

        // Binding the socket to the port
        if(::bind(socket_fd_, pointer->ai_addr, pointer->ai_addrlen) == -1){
          ::close(socket_fd_);
          rr::perror("Binding socket");
          continue;
        }
        break;
      }

      ::freeaddrinfo(results); // all done with this structure

      if(pointer == nullptr){
        rr::perror("Server failed to bind");
        if(++current_intent_ > max_intents_){
          rr::error("Max number of reconnections has been reached "
                    "and therefore failed to connect.");
          return;
        }
        rr::attention() << "Trying reconnection in " << reconnection_time_.count() / 1000
                        << " seconds... (#" << current_intent_ << ")" << rr::endl;
        std::this_thread::sleep_for(reconnection_time_);
        std::thread(&server::concurrent_connector, this, false).detach();
        return;
      }

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

      std::thread(&server::concurrent_connection, this, socket_fd_).detach();
    }

    void server::concurrent_connection(const int socket_fd){
      rr::attention("Waiting for incomming connections!");

      struct sockaddr_storage their_addr;
      socklen_t addr_size;
      int accepted_fd{0};

      while(!terminate_concurrent_){
        addr_size = sizeof(their_addr);
        // Accept incoming connection
        if((accepted_fd = ::accept(socket_fd, (struct sockaddr*)&their_addr, &addr_size)) == -1){
          rr::perror("Accepting connection");
          continue;
        }

        rr::attention("Connection established!");

        connected_ = true;
        connecting_ = false;
        connected_fd_ = accepted_fd;

        // Listening socket not needed anymore
        close();
        break;
      }
    }

    void server::concurrent_receive(char *buffer, ssize_t *size, const int flags){
      ssize_t total_received{0};
      ssize_t bytes_left = *size;
      ssize_t received_size;
      std::uint32_t error_counter{0};

      while(total_received < *size && !terminate_send_){
        if((received_size = ::recv(connected_fd_, buffer + total_received,
                                   static_cast<std::size_t>(bytes_left), flags)) == -1){
          rr::perror("Receiving data");
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          if(++error_counter > max_intents_){
            total_received = -1;
            break;
          }
        }
        total_received += received_size;
        bytes_left -= received_size;
      }
      *size = total_received;
    }

    void server::concurrent_send(const char *buffer, ssize_t *size, const int flags){
      ssize_t total_sent{0};
      ssize_t bytes_left = *size;
      ssize_t sent_size;
      std::uint32_t error_counter{0};

      while(total_sent < *size && !terminate_send_){
        if((sent_size = ::send(connected_fd_, buffer + total_sent,
                               static_cast<std::size_t>(bytes_left), flags)) == -1){
          rr::perror("Sending data");
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          if(++error_counter > max_intents_){
            total_sent = -1;
            break;
          }
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
