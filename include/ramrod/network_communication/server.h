#ifndef RAMROD_NETWORK_COMMUNICATION_SERVER_H
#define RAMROD_NETWORK_COMMUNICATION_SERVER_H

#include <chrono>
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

      bool connect(const std::string ip, const int port = 1313);
      bool disconnect();

      void max_queue(const int max_pending_connections = 10);

      uint16_t host_to_network(const uint16_t host_value);
      uint32_t host_to_network(const uint32_t host_value);

      const std::string& ip();
      bool is_connected();

      uint16_t mtu();
      void mtu(const uint16_t mtu_byte_size);

      uint16_t network_to_host(const uint16_t network_value);
      uint32_t network_to_host(const uint32_t network_value);

      int port();

      ssize_t receive(char *data, const std::size_t length);
      ssize_t receive(std::string *data, std::size_t length = 0, std::size_t stride = 0);

      bool reconnect();

      int time_to_reconnect();
      void time_to_reconnect(const int waiting_time_in_milliseconds);

      ssize_t send(const char *data, const std::size_t length);
      ssize_t send(const std::string &data, std::size_t length = 0, std::size_t stride = 0);

      bool pending_connection();

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
      uint16_t mtu_;
      bool is_jumbo_packet_;

      bool pending_connection_;
      std::chrono::duration<long, std::milli> reconnection_time_;
      std::chrono::time_point<std::chrono::system_clock> *beginning_;
    };
  } // namespace: network_communication
} // namespace: ramrod

#endif // RAMROD_NETWORK_COMMUNICATION_SERVER_H
