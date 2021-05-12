#include "ramrod/network_communication/server.h"

#include <ws2tcpip.h> // for getaddrinfo, addrinfo, freeaddrinfo

#include <cstring>                     // for memset
#include <iosfwd>                      // for size_t
#include <thread>                      // for sleep_for, thread

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
      socket_fd_(),
      connected_fd_(),
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
    {
      WSADATA wsa_data;

      // Initialize Winsock
      if(WSAStartup(MAKEWORD(2, 2), &wsa_data) != NO_ERROR){
        rr::error("Error at WSAStartup()");
        WSACleanup();
      }
    }

    server::~server(){
      disconnect();
      WSACleanup();
    }

    bool server::connect(const std::string &ip, const int port, const int socket_type,
                         const bool concurrent){
      if(connecting_) return false;
      if(connected_) disconnect();

      ip_ = ip;
      port_ = port;
      connecting_ = true;
      current_intent_ = 0;
      is_tcp_ = socket_type != SOCK_DGRAM;

      terminate_concurrent_ = false;

      if(concurrent)
        std::thread(&server::concurrent_connector, this, true, false).detach();
      else
        concurrent_connector(true, true);
      return true;
    }

    bool server::disconnect(){
      terminate_send_ = true;
      terminate_receive_ = true;
      terminate_concurrent_ = true;

      if(results_) ::freeaddrinfo(results_);
      results_ = nullptr;

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

    int server::receive(char *buffer, const int size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      int received{0}, from_length{static_cast<int>(client_->ai_addrlen)};

      if(is_tcp_)
        received = ::recv(connected_fd_, buffer, size, flags);
      else
        received = ::recvfrom(socket_fd_, buffer, size, flags,
                              client_->ai_addr, &from_length);
#ifdef VERBOSE
      if(received < 0) rr::perror("Receiving data " + std::to_string(socket_fd_) + " : " + std::to_string(connected_fd_));
#endif
      if(received == 0) disconnect();
      return received;
    }

    int server::receive_all(char *buffer, const int size, bool *breaker,
                                const int flags){
      if(!connected_ || size == 0)
        return 0;

      int total_received{0};
      int bytes_left = size;
      int received_size, from_length{static_cast<int>(client_->ai_addrlen)};
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      // TODO: change recvfrom so it doesn't modifies client_ and check if the data comes
      // from only for initial client_ and nobody elses
      while(total_received < size && !(*breaker)){
        if(is_tcp_)
          received_size = ::recv(connected_fd_, buffer + total_received, bytes_left, flags);
        else
          received_size = ::recvfrom(socket_fd_, buffer + total_received, bytes_left, flags,
                                     client_->ai_addr, &from_length);
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

    bool server::receive_all_concurrently(char *buffer, int *size, bool *breaker,
                                          const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_receive_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool server::receive_concurrently(char *buffer, int *size, const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_receive, this, buffer, size, flags).detach();
      return true;
    }

    bool server::reconnect(const bool concurrent){
      if(connecting_ || ip_.size() == 0 || port_ <= 0) return false;

      if(connected_) disconnect();

      connecting_ = true;
      current_intent_ = 0;
      terminate_concurrent_ = true;

      if(concurrent)
        std::thread(&server::concurrent_connector, this, true, false).detach();
      else
        concurrent_connector(true, true);
      return true;
    }

    int server::send(const char *buffer, const int size, const int flags){
      if(!connected_ || size == 0)
        return 0;

      int sent{::sendto(connected_fd_, buffer, size, flags, client_->ai_addr,
                        static_cast<int>(client_->ai_addrlen))};
#ifdef VERBOSE
      if(sent < 0) rr::perror("Sending data");
#endif
      if(sent == 0) disconnect();
      return sent;
    }

    int server::send_all(const char *buffer, const int size, bool *breaker,
                             const int flags){
      if(!connected_ || size == 0)
        return 0;

      int total_sent{0};
      int bytes_left = size;
      int sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < size && !(*breaker)){
        sent_size = ::sendto(connected_fd_, buffer + total_sent, bytes_left, flags,
                             client_->ai_addr, static_cast<int>(client_->ai_addrlen));

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

    bool server::send_all_concurrently(const char *buffer, int *size, bool *breaker,
                                       const int flags){
      if(!connected_ || *size == 0){
        *size = 0;
        return false;
      }

      std::thread(&server::concurrent_send_all, this, buffer, size, breaker, flags).detach();
      return true;
    }

    bool server::send_concurrently(const char *buffer, int *size, const int flags){
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

      if(::shutdown(socket_fd_, SD_BOTH) != NO_ERROR)
        rr::perror("Connection cannot be shutdown");

      if(::closesocket(socket_fd_) != NO_ERROR){
        rr::perror("Connection cannot be closed");
        return false;
      }

      return true;
    }

    bool server::close_child(){
      if(!is_tcp_){
        return !(connected_ = false);
      }

      if(connected_fd_ < 0){
#ifdef VERBOSE
        rr::warning("Children connection already closed.");
#endif
        return !(connected_ = false);
      }

      if(::shutdown(connected_fd_, SD_BOTH) != NO_ERROR)
        rr::perror("Children connection cannot be shutdown");

      if(::closesocket(connected_fd_) != NO_ERROR){
        rr::perror("Children connection cannot be closed");
        return !(connected_ = false);
      }

      return !(connected_ = false);
    }

    void server::concurrent_connector(const bool force, const bool wait){
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
                                  client_->ai_protocol)) != NO_ERROR){
          rr::perror("Selecting socket");
          continue;
        }

        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, (char*)&status, sizeof(int)) != NO_ERROR){
          rr::perror("Setting socket options");
          ::closesocket(socket_fd_);
          return;
        }

        // Binding the socket to the port
        if(::bind(socket_fd_, client_->ai_addr, static_cast<int>(client_->ai_addrlen)) != NO_ERROR){
          rr::perror("Binding socket");
          ::closesocket(socket_fd_);
          continue;
        }
        break;
      }

      if(is_tcp_){
        ::freeaddrinfo(results_); // all done with this structure
        results_ = nullptr;
      }

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

        if(wait)
          concurrent_connector(false, true);
        else
          std::thread(&server::concurrent_connector, this, false, false).detach();

        return;
      }

      if(is_tcp_)
        if(::listen(socket_fd_, max_queue_) != NO_ERROR){
          rr::perror("Listening to socket");
          return;
        }

      // If is UDP we wait for an incoming message that contains the client information
      if(!is_tcp_){
        sockaddr_storage receiver;
        socklen_t addrlen = sizeof(sockaddr_storage);
        char incoming[11];
        if(::recvfrom(socket_fd_, incoming, sizeof(incoming), 0,
                      (sockaddr*)&receiver, &addrlen) <= 0){
#ifdef VERBOSE
          rr::perror("Receiving identifier");
#endif
        }

        terminate_receive_ = false;
        terminate_send_ = false;
        connecting_ = false;

        if(::strcmp(incoming, "identifier") != 0){
#ifdef VERBOSE
          rr::error("Wrong identifier message");
#endif
          return;
        }
        client_->ai_addrlen = static_cast<size_t>(addrlen);
        std::memcpy(client_->ai_addr, &receiver, client_->ai_addrlen);
        connected_fd_ = socket_fd_;
        connected_ = true;
#ifdef VERBOSE
        rr::attention("Connection established!");
#endif
        return;
      }
      // In case is TCP then we must accept an incomming connection
      if(wait)
        concurrent_connection();
      else
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
        if((connected_fd_ = ::accept(socket_fd_, (struct sockaddr*)&their_addr, &addr_size))
           != NO_ERROR){
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

    void server::concurrent_receive(char *buffer, int *size, const int flags){
      int received{0}, from_length{static_cast<int>(client_->ai_addrlen)};
      if(is_tcp_)
        received = ::recv(connected_fd_, buffer, *size, flags);
      else
        received = ::recvfrom(socket_fd_, buffer, *size, flags,
                              client_->ai_addr, &from_length);
#ifdef VERBOSE
      if(received < 0) rr::perror("Receiving data");
#endif
      if(received == 0) disconnect();
      *size = received;
    }

    void server::concurrent_receive_all(char *buffer, int *size, bool *breaker,
                                        const int flags){
      int total_received{0};
      int bytes_left = *size;
      int received_size, from_length{static_cast<int>(client_->ai_addrlen)};
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_received < *size && !terminate_send_ && !(*breaker)){
        if(is_tcp_)
          received_size = ::recv(connected_fd_, buffer + total_received, bytes_left, flags);
        else
          received_size = ::recvfrom(socket_fd_, buffer + total_received, bytes_left, flags,
                                     client_->ai_addr, &from_length);
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

    void server::concurrent_send(const char *buffer, int *size, const int flags){
      int sent{::sendto(connected_fd_, buffer, *size, flags,
                            client_->ai_addr, static_cast<int>(client_->ai_addrlen))};
#ifdef VERBOSE
      if(sent < 0) rr::perror("Sending data");
#endif
      if(sent == 0) disconnect();
      *size = sent;
    }

    void server::concurrent_send_all(const char *buffer, int *size, bool *breaker,
                                     const int flags){
      int total_sent{0};
      int bytes_left = *size;
      int sent_size;
      std::uint32_t error_counter{0};
      bool never{false};
      if(breaker == nullptr) breaker = &never;

      while(total_sent < *size && !terminate_send_ && !(*breaker)){
        sent_size = ::sendto(connected_fd_, buffer + total_sent, bytes_left, flags,
                             client_->ai_addr, static_cast<int>(client_->ai_addrlen));
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
  } // namespace: network_communication
} // namespace: ramrod
