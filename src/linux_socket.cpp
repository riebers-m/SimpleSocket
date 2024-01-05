//
// Created by max on 03.01.24.
//
#include "simple_socket.hpp"
#include "internal/simple_types.hpp"
#include <cerrno>
#include <unistd.h>
#include <fmt/format.h>
#include <fmt/color.h>
#include <netdb.h>
#include <thread>
#include <poll.h>

namespace simple {
    static constexpr auto DEFAULT_BUFFER_SIZE = 1024;
    using namespace std::chrono_literals;

    static std::stop_source s_stop_token{};
    static std::jthread s_worker;

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

    TcpSocket TcpSocket::connect(std::string_view const host, const std::string_view port) {
        socket_t sock = 0;
        struct addrinfo hints = {0};
        struct addrinfo *result, *rp = nullptr;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          /* Any protocol */

        if (auto ret = getaddrinfo(host.data(), port.data(), &hints, &result);ret != 0) {
            throw SocketError(fmt::format("could not resolve address: {}", gai_strerror(ret)));
        }

        for (rp = result; rp != nullptr; rp = rp->ai_next) {
            sock = socket(rp->ai_family, rp->ai_socktype,
                          rp->ai_protocol);
            if (sock == -1)
                continue;

            if (::connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
                break;                  /* Success */

            ::close(sock);
        }

        freeaddrinfo(result);           /* No longer needed */

        if (rp == nullptr) {               /* No address succeeded */
            throw SocketError("could not connect");
        }
        return TcpSocket{sock};
    }

    TcpSocket::TcpSocket(simple::socket_t t_sock) :
            m_socket{t_sock},
            m_buffer_size{DEFAULT_BUFFER_SIZE} {}

    TcpSocket::~TcpSocket() {
        shutDown();
        if(s_worker.joinable()) {
            s_worker.join();
        }
        close();
    }

    void TcpSocket::send(std::string_view const message) {
        send(std::vector<char>{message.cbegin(), message.cend()});
    }

    void TcpSocket::send(std::vector<char> const &message) {
        if(message.empty()) {return;}

        ssize_t sent_bytes = 0;
        const char* send_ptr = message.data();
        auto tries = 3;

        std::lock_guard lock{m_mutex};
        while(sent_bytes<message.size() && tries > 0) {
            ssize_t sent = 0;
            if(sent = ::send(m_socket, send_ptr + sent_bytes, message.size() - sent_bytes, 0); sent == -1) {
                throw SocketError(fmt::format("could not send message: {}", strerror(errno)));
            }
            sent_bytes += sent;
            tries--;
        }
    }

    void TcpSocket::setReceiveCallback(ReceiveCallback const &callback) {
        m_callback = callback;
    }

    void TcpSocket::run() {
        s_worker = std::jthread([&](std::stop_token stop_token) {
            if(!m_callback) {
                fmt::print("empty callback shutting down");
                return;
            }

            pollfd fds[1];
            fds[0].fd = m_socket;
            fds[0].events = POLLIN;


            while(!stop_token.stop_requested()) {
                auto result = 0;
                if(result = poll(fds, 1, 10);result==-1) {
                    fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "poll error: {}", strerror(errno));
                    return;
                } else if(result == 0) {
                    continue;
                }

                if(m_callback) {
                    fmt::print("reading from socket\n");
                    std::lock_guard lock{m_mutex};
                    m_callback(read());
                }
            }
            fmt::print("worker thread shutting down");
        });
        s_stop_token = s_worker.get_stop_source();
    }

    void TcpSocket::shutDown() {
        std::lock_guard lock{m_mutex};
        s_stop_token.request_stop();
    }
}