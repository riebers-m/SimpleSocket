//
// Created by max on 03.01.24.
//
#include "simple_socket.hpp"
#include "internal/simple_types.hpp"
#include <cerrno>
#include <unistd.h>
#include <fmt/format.h>

namespace simple {
    static constexpr auto DEFAULT_BUFFER_SIZE = 1024;

    TcpSocket::TcpSocket() :
    m_socket{-1},
    m_mutex{},
    m_buffer_size{DEFAULT_BUFFER_SIZE},
    m_peer{},
    m_is_open{false}
    {}

    std::vector<char> TcpSocket::read() {
        if(!m_is_open) {
            throw SocketError(fmt::format("socket not open"));
        }

        std::vector<char> buffer(m_buffer_size);

        std::lock_guard lock{m_mutex};

        const auto read = recv(m_socket, buffer.data(), buffer.size(), 0);

        if (read == 0) {
            throw SocketError("peer has shutdown connection");
        }
        if (read == -1) {
            throw SocketError(fmt::format("reading from socket failed: {}", strerror(errno)));
        }
        return buffer;
    }

    void TcpSocket::close() {
        if (::close(m_socket) == -1) {
            throw SocketError(fmt::format("closing socket failed: {}", strerror(errno)));
        }
        setIsOpen(false);
    }

    TcpSocket::TcpSocket(socket_t t_sock, Peer const &t_peer) :
            m_socket{t_sock},
            m_buffer_size{DEFAULT_BUFFER_SIZE},
            m_peer(t_peer) {}

    void TcpSocket::send(std::string_view const message) {
        send(std::vector<char>{message.cbegin(), message.cend()});
    }

    void TcpSocket::send(std::vector<char> const &message) {
        if (message.empty()) { return; }
        if(!m_is_open) {
            throw SocketError(fmt::format("socket not open"));
        }

        ssize_t sent_bytes = 0;
        const char *send_ptr = message.data();
        auto tries = 3;

        std::lock_guard lock{m_mutex};
        while (sent_bytes < message.size() && tries > 0) {
            ssize_t sent = 0;
            if (sent = ::send(m_socket, send_ptr + sent_bytes, message.size() - sent_bytes, 0); sent == -1) {
                throw SocketError(fmt::format("could not send message: {}", strerror(errno)));
            }
            sent_bytes += sent;
            tries--;
        }
    }

    Peer const &TcpSocket::getPeer() const {
        return m_peer;
    }

    bool TcpSocket::isOpen() const {
        return m_is_open;
    }

    void TcpSocket::setIsOpen(bool isOpen) {
        m_is_open = isOpen;
    }

    void TcpSocket::setPeer(Peer const &peer) {
        m_peer = peer;
    }


}