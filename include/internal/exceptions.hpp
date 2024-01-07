//
// Created by max on 03.01.24.
//

#ifndef SIMPLESOCKET_EXCEPTIONS_HPP
#define SIMPLESOCKET_EXCEPTIONS_HPP

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <filesystem>

namespace simple {
    class SocketError : protected std::exception {
    public:
        explicit SocketError(std::string_view const message, std::filesystem::path const &file, auto line ) :
        m_message{fmt::format("{}:{}: {}", file.filename().string(), line, message)} {}

        [[nodiscard]] const char *what() const noexcept override {
            return m_message.c_str();
        }
    protected:
        std::string m_message;
    };

    class SocketShutdownError : public SocketError {
    public:
        explicit SocketShutdownError(std::string_view const message, std::string_view const file, auto line ) :
                SocketError(message, file, line) {}

    };

#define SocketError(message) SocketError{message, __FILE__, __LINE__}
#define SocketShutdownError(message) SocketShutdownError{message, __FILE__, __LINE__}
}
#endif //SIMPLESOCKET_EXCEPTIONS_HPP
