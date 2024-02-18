//
// Created by max on 11.02.24.
//

#ifndef SIMPLESOCKET_SIMPLE_SOCKETS_HPP
#define SIMPLESOCKET_SIMPLE_SOCKETS_HPP

#include <utility>

#include "simple_socket.hpp"

namespace simple {
    class Sockets final {
    private:
        Sockets();

        static Sockets const &instance();

    public:
        Sockets(Sockets const &) = delete;

        Sockets(Sockets &&) noexcept = delete;

        Sockets &operator=(Sockets const &) = delete;

        Sockets &operator=(Sockets &&) noexcept = delete;

        ~Sockets();

        static Client create_client(
                std::string const &host,
                std::uint16_t port,
                Client::ReceiveCallback callback,
                Sockets const & = instance()
        );

        static ServerSocket create_server(
                std::uint16_t port,
                ServerSocket::blocking accept_blocking,
                std::chrono::milliseconds const &accept_timeout = std::chrono::milliseconds(1),
                Sockets const& = instance());
    };
}
#endif //SIMPLESOCKET_SIMPLE_SOCKETS_HPP
