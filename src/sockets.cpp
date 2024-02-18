//
// Created by max on 16.02.24.
//
#include "simple_sockets.hpp"

namespace simple {
    Sockets::Sockets() {
        // e.g. invoke global initialization here
    }

    Sockets::~Sockets() {

    }

    Sockets const &Sockets::instance() {
        static auto handle = Sockets{};
        return handle;
    }

    Client Sockets::create_client(std::string const &host, std::uint16_t port, Client::ReceiveCallback callback,
                                  Sockets const &) {
        return Client{host, port, std::move(callback)};
    }

    ServerSocket
    Sockets::create_server(
            std::uint16_t port,
            ServerSocket::blocking accept_blocking,
            std::chrono::milliseconds const &accept_timeout,
            Sockets const&) {
        return ServerSocket{port, accept_blocking, accept_timeout};
    }


}
