//
// Created by max on 03.01.24.
//

#ifndef SIMPLESOCKET_EXCEPTIONS_HPP
#define SIMPLESOCKET_EXCEPTIONS_HPP

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>

namespace simple {
    class SocketError : protected std::exception {
    public:
        explicit SocketError(std::string_view const message, std::string_view const file, auto line ) :
        m_message{fmt::format("{}:{}: {}", file, line, message)} {}

        [[nodiscard]] const char *what() const noexcept override {
            return m_message.c_str();
        }
    protected:
        std::string m_message;
    };

#define SocketError(message) SocketError{message, __FILE__, __LINE__}
}
#endif //SIMPLESOCKET_EXCEPTIONS_HPP
