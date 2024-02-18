#pragma once

#include <vector>
#include <string>
#include <functional>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <mutex>
#include "internal/exceptions.hpp"
#include "internal/simple_types.hpp"
#include "internal/unique_value.hpp"

namespace simple {
    struct Peer {
        std::string host;
        std::uint16_t port;
    };

    class BaseSocket {
    protected:
        using unique_deleter = void(*)(socket_t);
        UniqueValue<socket_t, unique_deleter> m_socket;
        bool m_is_open;
        explicit BaseSocket(socket_t socket);
        BaseSocket(socket_t, unique_deleter);
    public:
        [[nodiscard]] std::optional<socket_t> socket_handle() const {
            if(m_socket.has_value()) {
                return m_socket.value();
            }
            return std::nullopt;
        }
    };

    class Client : public BaseSocket {
        friend class Sockets;
        friend class ServerSocket;
    public:
        using ReceiveCallback = std::function<std::vector<char>(std::vector<char> const & request)>;

        Client(Client &&) noexcept;
        Client& operator=(Client &&) noexcept;
        ~Client();

        std::size_t send(std::string_view const message);
        std::size_t send(std::vector<char> const &message);

        [[nodiscard]] bool is_open() const;
        void close();

    private:
        explicit Client(socket_t, ReceiveCallback callback, Peer const &peer);
        Client(socket_t, Client::ReceiveCallback const &);
        Client(std::string const &host, std::uint16_t port, ReceiveCallback callback);

        std::vector<char> receive() const;
        std::string receive_string() const;
        void waiting_for_incoming_message(std::stop_token const&);

    private:
        bool m_is_open;
        Peer m_peer;
    public:
        Peer const &getPeer() const;

    private:
        ReceiveCallback m_callback;
        std::mutex mutable m_mutex;
        std::jthread m_worker;
    };

    class ServerSocket : public BaseSocket {
        friend class Sockets;

    public:
        enum class blocking {
            not_blocking,
            blocking
        };

        ServerSocket(ServerSocket &&) noexcept = default;
        ServerSocket& operator=(ServerSocket &&) noexcept = default;

        [[nodiscard]] Client accept(const Client::ReceiveCallback& callback);
        [[nodiscard]] bool is_open() const;
        void close();

    private:
        std::chrono::milliseconds m_accept_timeout{1};
        bool m_is_open;

        explicit ServerSocket(std::uint16_t port, blocking accept_blocking,
                              std::chrono::milliseconds const &accept_timeout = std::chrono::milliseconds(1));


    };
}