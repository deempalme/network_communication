#ifndef RAMROD_SOCKET_ERROR_HANDLER_HPP
#define RAMROD_SOCKET_ERROR_HANDLER_HPP

#include "ramrod/socket/Enumerators.hpp" // for ErrorType

#include <array> // for array

namespace ramrod::socket
{
    class ErrorHandler
    {
    public:
        ErrorHandler() = default;
        virtual ~ErrorHandler() = default;

        /**
         * @brief Get a standard \b errno code from a \b ErrorType enum.
         *
         * @param[in] error_type  ErrorType enum returned by a ramrod::socket function
         *
         * @return A standard errno, some values in enum may not be equivalent to an errno
         *         in such case, the last value in errno when ErrorType was encountered is
         *         returned and it may not match with \p error_type, if the value is zero
         *         then, it means that \p error_type does not have a \b errno
         */
        int get_errno(const ErrorType error_type);

        /**
         * @brief Get the full description of the encountered error.
         *
         * @param[in] error_type  Error code returned by a function
         *
         * @return String with the detailed description of the encountered error
         */
        const char *get_error_detail(const ErrorType error_type);

    protected:
        /**
         * @brief Set the error code for a defined ErrorType.
         *
         * @param[in] error_type  ErrorType enum returned by a ramrod::socket function
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return Same value than \p error_type (passthrough)
         */
        ErrorType set_error_code(const ErrorType error_type, const int error_code);

        /**
         * @brief Set the error code for a standard errno.
         *
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return ErrorType enum equivalent to errno
         */
        ErrorType set_error_code(const int error_code);

    private:
        /**
         * @brief Get ErrorType from a standard socket's returned code.
         *
         * @param[in] error_code  Error code returned by a standard socket function
         *
         * @return An error enum compatible with ramrod::socket
         */
        ErrorType get_error_type(const int error_code);

        /// @brief Stores the lastest error codes for each ErrorType
        std::array<int, static_cast<size_t>(ErrorType::UNKNOWN_ERROR)> error_codes_{};
    };
} // namespace: ramrod::socket

#endif // RAMROD_SOCKET_ERROR_HANDLER_HPP
