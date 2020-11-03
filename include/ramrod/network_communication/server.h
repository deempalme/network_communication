#ifndef RAMROD_NETWORK_COMMUNICATION_SERVER_H
#define RAMROD_NETWORK_COMMUNICATION_SERVER_H

#include <cstdarg>
#include <cstdio>
#include <string>

struct addrinfo;
struct sockaddr;
struct sockaddr_storage;

namespace ramrod {
  namespace network_communication {
    class server
    {
    public:
      server();
      ~server();

      bool connect(const std::string ip = "127.1.0.13", const int port = 1313);
      bool disconnect();

      void max_queue(const int max_pending_connections = 10);

      uint16_t host_to_network(const uint16_t host_value);
      uint32_t host_to_network(const uint32_t host_value);

      const std::string& ip();

      short mtu();
      void mtu(const short mtu_byte_size = 1500);

      uint16_t network_to_host(const uint16_t network_value);
      uint32_t network_to_host(const uint32_t network_value);

      int port();

      ssize_t receive(void *data, const std::size_t length);
      ssize_t receive(std::string *data, std::size_t length = 0, std::size_t stride = 0);

      ssize_t send(const void *data, const std::size_t length);
      ssize_t send(const std::string &data, std::size_t length = 0, std::size_t stride = 0);

    protected:
      void print_error(const std::string &message);

      int formatted(const char *format, ...) __attribute__((format (printf, 2, 3)));

    private:
      void *get_in_addr(struct sockaddr *sa);

      std::string ip_;
      int port_;
      int socket_fd_;
      struct addrinfo *client_;
      struct sockaddr_storage *last_receiver_;
      const std::size_t max_buffer_length_;
      bool connected_;
      short mtu_;
      bool is_jumbo_packet_;

    };
  } // namespace: network_communication
} // namespace: ramrod

#endif // RAMROD_NETWORK_COMMUNICATION_SERVER_H
