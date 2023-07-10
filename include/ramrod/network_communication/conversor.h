#ifndef RAMROD_NETWORK_COMMUNICATION_CONVERSOR_H
#define RAMROD_NETWORK_COMMUNICATION_CONVERSOR_H

#include <cstdint>  // for uint64_t, uint32_t, uint16_t

namespace ramrod::network_communication {
  class conversor
  {
  public:
    conversor() = default;
    /**
     * @brief Converting a 16 bit integer value stored in a big/little endian
     *        machine into the network's endian type
     *
     * @param host_value Unsigned 16 bit integer value to be converted
     *
     * @return The 16 bit `host_value` converted into the network's endian type
     */
    std::uint16_t host_to_network(const std::uint16_t host_value);
    /**
     * @brief Converting a 32 bit integer value stored in a big/little endian
     *        machine into the network's endian type
     *
     * @param host_value Unsigned 32 bit integer value to be converted
     *
     * @return The 32 bit `host_value` converted into the network's endian type
     */
    std::uint32_t host_to_network(const std::uint32_t host_value);
    /**
     * @brief Converting a 64 bit integer value stored in a big/little endian
     *        machine into the network's endian type
     *
     * @param host_value Unsigned 64 bit integer value to be converted
     *
     * @return The 64 bit `host_value` converted into the network's endian type
     */
    std::uint64_t host_to_network(const std::uint64_t host_value);
    /**
     * @brief Converting a 16 bit integer value stored in the network's endian type
     *        into the endian type that your computer uses
     *
     * @param network_value Unsigned 16 bit integer value to be converted
     *
     * @return The 16 bit `host_value` converted into your computer's endian type
     */
    std::uint16_t network_to_host(const std::uint16_t network_value);
    /**
     * @brief Converting a 32 bit integer value stored in the network's endian type
     *        into the endian type that your computer uses
     *
     * @param network_value Unsigned 32 bit integer value to be converted
     *
     * @return The 32 bit `host_value` converted into your computer's endian type
     */
    std::uint32_t network_to_host(const std::uint32_t network_value);
    /**
     * @brief Converting a 64 bit integer value stored in the network's endian type
     *        into the endian type that your computer uses
     *
     * @param network_value Unsigned 64 bit integer value to be converted
     *
     * @return The 64 bit `host_value` converted into your computer's endian type
     */
    std::uint64_t network_to_host(const std::uint64_t network_value);

  private:
  };
} // namespace ramrod::network_communication

#endif // RAMROD_NETWORK_COMMUNICATION_CONVERSOR_H
