#pragma once

#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include "internal/exceptions.hpp"
#include "internal/simple_types.hpp"

namespace simple {
    struct Peer {
        std::string host;
        std::string service;
    };

    class TcpSocket {
    public:
        TcpSocket();

        void send(std::string_view const message);
        void send(std::vector<char> const &message);
        [[nodiscard]] Peer const &getPeer() const;
        void setPeer(Peer const &peer);
        [[nodiscard]] bool isOpen() const;
        void setIsOpen(bool isOpen);

    protected:
        explicit TcpSocket(socket_t sock, Peer const &t_peer);
        [[nodiscard]] std::vector<char> read();
        void close();
    protected:
        socket_t m_socket;
        std::mutex m_mutex;
    private:
        std::size_t m_buffer_size;
        Peer m_peer;
        bool m_is_open;
    };

    class ClientSocket : public TcpSocket {
    public:
        using ReceiveCallback = std::function<std::vector<char>(std::vector<char> const & request)>;

        ClientSocket();
        explicit ClientSocket(socket_t t_sock, Peer const &t_peer, bool t_is_open=true);
        virtual ~ClientSocket();

        void connect(std::string_view const host, std::string_view const port);
        void setReceiveCallback(ReceiveCallback const &callback);
        void run();
        void stop();
        void shutDown();
    private:
        ReceiveCallback m_callback;
    };

    class ServerSocket : public TcpSocket {
    public:
        using AcceptCallback = std::function<void(std::vector<char> const &)>;

        ServerSocket();
        virtual ~ServerSocket();

        void bindAndListen(std::string_view port, int t_backlog= 5);
        void setAcceptCallback(ServerSocket::AcceptCallback const &accept_callback);
        ClientSocket accept();
    private:
        AcceptCallback m_accept_callback;
        std::atomic<bool> m_runnig{false};
    };
}