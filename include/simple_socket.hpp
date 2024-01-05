#pragma once

#include <vector>
#include <string>
#include <functional>
#include "internal/exceptions.hpp"
#include "internal/simple_types.hpp"

namespace simple {
    using ReceiveCallback = std::function<void(std::vector<char> const &)>;

    class TcpSocket {
    public:
        struct Peer {
            std::string host;
            std::string service;
        };

        static TcpSocket connect(std::string_view host, std::string_view port);

        virtual ~TcpSocket();

        void send(std::string_view const message);
        void send(std::vector<char> const & message);
        void setReceiveCallback(ReceiveCallback const &callback);

        void run();
        void shutDown();

    protected:
        [[nodiscard]] std::vector<char> read() const;
        void close() const;
        ReceiveCallback  m_callback;

    private:
        explicit TcpSocket(socket_t sock);
        socket_t m_socket;
        std::size_t m_buffer_size;
        Peer m_peer;
        std::mutex m_mutex;
    };
}