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

    std::vector<char> TcpSocket::read() const {
        std::vector<char> buffer(m_buffer_size);

        const auto read = recv(m_socket, buffer.data(), buffer.size(), 0);

        if (read == 0) {
            throw SocketError("peer has shutdown connection");
        }
        if (read == -1) {
            throw SocketError(fmt::format("reading from socket failed: {}", strerror(errno)));
        }
        return buffer;
    }

    void TcpSocket::close() const {
        if (::close(m_socket) == -1) {
            throw SocketError(fmt::format("closing socket failed: {}", strerror(errno)));
        }
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

//    TcpSocket TcpSocket::listenAndBind(std::string_view port) {
//        struct addrinfo hints = {0};
//        struct addrinfo *result, *rp = nullptr;
//        int sfd, s;
//        struct sockaddr_storage peer_addr = {0};
//        socklen_t peer_addr_len;
//
//        memset(&hints, 0, sizeof(hints));
//        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
//        hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
//        hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
//        hints.ai_protocol = 0;          /* Any protocol */
//        hints.ai_canonname = nullptr;
//        hints.ai_addr = nullptr;
//        hints.ai_next = nullptr;
//
//        s = getaddrinfo(nullptr, port.data(), &hints, &result);
//        if (s != 0) {
//            throw SocketError(fmt::format("listenAndBind getaddrinfo failed: {}", gai_strerror(s)));
//        }
//
//        /* getaddrinfo() returns a list of address structures.
//           Try each address until we successfully bind(2).
//           If socket(2) (or bind(2)) fails, we (close the socket
//           and) try the next address. */
//
//        for (rp = result; rp != nullptr; rp = rp->ai_next) {
//            sfd = socket(rp->ai_family, rp->ai_socktype,
//                         rp->ai_protocol);
//            if (sfd == -1)
//                continue;
//
//            if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
//                break;                  /* Success */
//
//            ::close(sfd);
//        }
//
//        return TcpSocket(sfd, TcpSocket::Peer());
//    }
}