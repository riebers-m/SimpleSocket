#pragma once

#include <vector>
#include <string>
#include <functional>
#include "internal/exceptions.hpp"
#include "internal/simple_types.hpp"

namespace simple {


    struct Peer {
        std::string host;
        std::string service;
    };

    class TcpSocket {
    public:

        void send(std::string_view const message);
        void send(std::vector<char> const &message);
    protected:

    //    static TcpSocket listenAndBind(std::string_view port);
        [[nodiscard]] std::vector<char> read() const;

        void close() const;
        [[nodiscard]] Peer const &getPeer() const;

        explicit TcpSocket(socket_t sock, Peer const &t_peer);

    protected:
        socket_t m_socket;
        std::mutex m_mutex;
    private:
        std::size_t m_buffer_size;
        Peer m_peer;
    };

    class ClientSocket : public TcpSocket {
    public:
        using ReceiveCallback = std::function<void(std::vector<char> const &)>;

        static ClientSocket connect(std::string_view host, std::string_view port);

        void setReceiveCallback(ReceiveCallback const &callback);

        void run();

        void shutDown();

    private:
        explicit ClientSocket(socket_t t_sock, Peer const &t_peer);

        ReceiveCallback m_callback;

    };
}