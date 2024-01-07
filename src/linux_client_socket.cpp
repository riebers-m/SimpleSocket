//
// Created by max on 05.01.24.
//
#include <thread>
#include "simple_socket.hpp"
#include "fmt/color.h"

namespace simple {
    static std::stop_source s_stop_token{};
    static std::jthread s_worker;

    ClientSocket::ClientSocket() : TcpSocket() {

    }

    void ClientSocket::connect(std::string_view const host, std::string_view const port) {
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
            m_socket = socket(rp->ai_family, rp->ai_socktype,
                          rp->ai_protocol);
            if (m_socket == -1)
                continue;

            if (::connect(m_socket, rp->ai_addr, rp->ai_addrlen) != -1)
                break;                  /* Success */

            ::close(m_socket);
        }

        freeaddrinfo(result);           /* No longer needed */

        if (rp == nullptr) {               /* No address succeeded */
            throw SocketError("could not connect");
        }
        setPeer({std::string{host}, std::string{port}});
        setIsOpen(true);
    }

    void ClientSocket::setReceiveCallback(ReceiveCallback const &callback) {
        m_callback = callback;
    }

    void ClientSocket::run() {
        s_worker = std::jthread([&](std::stop_token stop_token) {
            if (!m_callback) {
                fmt::print("empty callback shutting down");
                return;
            }

            pollfd fds[1];
            fds[0].fd = m_socket;
            fds[0].events = POLLIN;


            while (!stop_token.stop_requested()) {
                if(!isOpen()) {
                    throw SocketError(fmt::format("socket {} not open", m_socket));
                }
                auto result = 0;
                if (result = poll(fds, 1, 10);result == -1) {
                    fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "poll error: {}", strerror(errno));
                    return;
                } else if (result == 0) {
                    continue;
                }

                if (m_callback) {
                    // TODO: remove this
                    fmt::print("reading from socket\n");
                    try {
                        if(const auto response = m_callback(read());!response.empty()) {
                            send(response);
                        }
                    } catch(SocketShutdownError const & e) {
                        fmt::print("{}", e.what());
                        setIsOpen(false);
                        return;
                    } catch(SocketError const & e) {
                        fmt::print("communication error on socket {}: {}", m_socket, e.what());
                    }
                }
            }
            fmt::print("worker thread shutting down");
        });
        s_stop_token = s_worker.get_stop_source();
    }

    void ClientSocket::shutDown() {
        std::lock_guard lock{m_mutex};
        if(s_stop_token.stop_possible()) {
            s_stop_token.request_stop();
        }
    }

    ClientSocket::ClientSocket(socket_t t_sock, Peer const &t_peer, bool t_is_open) : TcpSocket(t_sock, t_peer) {
        setIsOpen(t_is_open);
    }

    ClientSocket::~ClientSocket() {
        shutDown();
        if(s_worker.joinable()) {
            s_worker.join();
        }
        close();
    }

    void ClientSocket::stop() {
        shutDown();
    }
}