#include "ramrod/network_communication/server.h"

#include <arpa/inet.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace ramrod {
  namespace network_communication {
    server::server() :
      ip_(),
      port_{1313},
      socket_fd_{0},
      client_{nullptr},
      last_receiver_{nullptr},
      max_buffer_length_{100},
      connected_{false},
      mtu_{1500},
      is_jumbo_packet_{false},
      pending_connection_{false},
      reconnection_time_(std::chrono::milliseconds(5000)),
      beginning_{nullptr}
    {}

    server::~server(){
      disconnect();
      if(beginning_) delete beginning_;
    }

    bool server::connect(const std::string ip, const int port){
      ip_ = ip;
      port_ = port;
      pending_connection_ = true;

      return reconnect();
      /*
      rr::attention("Server: waiting for recvfrom...");

      struct sockaddr_storage their_addr;
      socklen_t addr_len{sizeof(their_addr)};
      ssize_t num_bytes;
      char buffer[max_buffer_length_];
      char s[INET6_ADDRSTRLEN];

      if((num_bytes = ::recvfrom(socket_fd_, buffer, max_buffer_length_ - 1, 0,
                                (struct sockaddr *)&their_addr, &addr_len)) == -1){
        ::perror("recvfrom");
        return false;
      }

      rr::formatted("Listener: got packet from %s\n", rr::message::message,
                    ::inet_ntop(their_addr.ss_family,
                                get_in_addr((struct sockaddr *)&their_addr),
                                s, sizeof(s)));
      rr::formatted("Listener: packet is %ld bytes long\n", rr::message::message, num_bytes);
      buffer[num_bytes] = '\0';
      rr::formatted("Listened: packet contains \"%s\"\n", buffer);
      */
    }

    bool server::disconnect(){
      if(client_ != nullptr) ::freeaddrinfo(client_);

      if(::close(socket_fd_) < 0){
        print_error("Connection cannot be closed.");
        return false;
      }
      return !(connected_ = false);
    }

    uint16_t server::host_to_network(const uint16_t host_value){
      return ::htons(host_value);
    }

    uint32_t server::host_to_network(const uint32_t host_value){
      return ::htonl(host_value);
    }

    const std::string &server::ip(){
      return ip_;
    }

    bool server::is_connected(){
      return connected_;
    }

    uint16_t server::mtu(){
      return mtu_;
    }

    void server::mtu(const uint16_t mtu_byte_size){
      mtu_ = mtu_byte_size;
      is_jumbo_packet_ = mtu_ >= 7700;
    }

    uint16_t server::network_to_host(const uint16_t network_value){
      return ::ntohs(network_value);
    }

    uint32_t server::network_to_host(const uint32_t network_value){
      return ::ntohl(network_value);
    }

    int server::port(){
      return port_;
    }

    ssize_t server::receive(char *data, const std::size_t length){
      socklen_t addrlen;
      ssize_t size{static_cast<ssize_t>(length)};
      ssize_t total{0};
      ssize_t bytes_left{size};
      ssize_t bytes_received;

      while(total < size){
        bytes_received = ::recvfrom(socket_fd_, data + total,
                                    static_cast<std::size_t>(bytes_left), 0,
                                    (struct sockaddr *)last_receiver_, &addrlen);
        if(bytes_received == 0){
          print_error("Connection closed when receiving");
          break;
        }else if(bytes_received == -1){
          ::perror("Error while receiving data");
          break;
        }
        total += bytes_received;
        bytes_left -= bytes_received;
      }

      return total;
    }

    bool server::reconnect(){
      beginning_ =
          new std::chrono::time_point<std::chrono::system_clock>(std::chrono::system_clock::now());

      struct addrinfo hints, *client_info;
      int addrinfo_err;
      int yes = 1;

      // first, load up address structs with getaddrinfo():

      memset(&hints, 0, sizeof(hints));
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_flags = AI_PASSIVE;

      if((addrinfo_err = ::getaddrinfo(ip_.c_str(), std::to_string(port_).c_str(),
                                       &hints, &client_info)) != 0){
        formatted("getaddrinfo: %s\n", ::gai_strerror(addrinfo_err));
        return false;
      }

      // loop through all the results and bind to the first we can
      for(client_ = client_info; client_ != nullptr; client_ = client_->ai_next){
        if((socket_fd_ = ::socket(client_->ai_family, client_->ai_socktype,
                                  client_->ai_protocol)) == -1){
          ::perror("Error selecting socket");
          continue;
        }

        if(::setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
          ::perror("Error setting socket options");
          return false;
        }

        if(::bind(socket_fd_, client_->ai_addr, client_->ai_addrlen) == -1){
          ::close(socket_fd_);
          ::perror("Error binding socket");
          return false;
        }
        break;
      }

      ::freeaddrinfo(client_info); // all done with this structure

      if(client_ == nullptr){
        print_error("Error: server failed to bind.");
        return connected_ = false;
      }

      delete beginning_;
      return connected_ = !(pending_connection_ = false);
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

    ssize_t server::send(const char *data, const std::size_t length){
      ssize_t size{static_cast<ssize_t>(length)};
      ssize_t total{0};
      ssize_t bytes_left{size};
      ssize_t bytes_sent;

      while(total < size){
        bytes_sent = ::sendto(socket_fd_, data + total,
                              static_cast<std::size_t>(bytes_left), MSG_DONTROUTE,
                              client_->ai_addr, client_->ai_addrlen);
        if(bytes_sent == 0){
          print_error("Connection closed when sending");
          break;
        }else if(bytes_sent == -1){
          ::perror("Error while sending data");
          break;
        }
        total += bytes_sent;
        bytes_left -= bytes_sent;
      }

      return total;
    }

    bool server::pending_connection(){
      if(!pending_connection_) return false;

      if((std::chrono::system_clock::now() - *beginning_) < reconnection_time_) return true;

      return !reconnect();
    }

    // ::::::::::::::::::::::::::::::::::: PROTECTED FUNCTIONS :::::::::::::::::::::::::::::::::::

    void server::print_error(const std::string &message){
      std::cout << "\033[1;41m Error: \033[0;1;38;5;174m " << message << "\033[0m" << std::endl;
    }

    int server::formatted(const char *format, ...){
      std::cout << "\033[0;1;38;5;174m";

      va_list arg;
      va_start(arg, format);
      const int result{std::vprintf(format, arg)};
      va_end(arg);

      std::cout << "\033[0m";
      std::flush(std::cout);

      return result;
    }

    // :::::::::::::::::::::::::::::::::::: PRIVATE FUNCTIONS ::::::::::::::::::::::::::::::::::::

    void *server::get_in_addr(sockaddr *sa){
      // get sockaddr, IPv4 or IPv6:
      if(sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
      return &(((struct sockaddr_in6 *)sa)->sin6_addr);
    }
  } // namespace: network_communication
} // namespace: ramrod
