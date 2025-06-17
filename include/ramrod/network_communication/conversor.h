#ifndef RAMROD_SOCKET_CONVERSOR_HPP
#define RAMROD_SOCKET_CONVERSOR_HPP

namespace ramrod::socket
{
    class Conversor
    {
    public:
        Conversor() = default;
        virtual ~Conversor() = default;

        /**
         * @brief Converting an unsigned integer value stored in a big/little endian
         *        machine into the network's endian type.
         *
         * Available types for conversion:
         * - uint16_t
         * - uint32_t
         * - uint64_t
         *
         * @param[in] value  Unsigned integer value to be converted
         *
         * @return The \p value converted into the network's endian type
         */
        template <typename T>
        T host_to_network(const T value);

        /**
         * @brief Converting an unsigned integer value stored in the network's endian
         *        type into the endian type that your computer uses.
         *
         * Available types for conversion:
         * - uint16_t
         * - uint32_t
         * - uint64_t
         *
         * @param[in] value  Unsigned integer value to be converted
         *
         * @return The \p value converted into your computer's endian type
         */
        template <typename T>
        T network_to_host(const T value);
    };
} // namespace ramrod::socket

#endif // RAMROD_SOCKET_CONVERSOR_HPP
