//
// Created by max on 05.01.24.
//
#include "simple_socket.hpp"

#include <thread>

namespace simple {
    ServerSocket::ServerSocket() : TcpSocket() {

    }

    void ServerSocket::bindAndListen(std::string_view port, int t_backlog, bool t_non_blocking) {
        struct addrinfo hints = {0};
        struct addrinfo *result, *rp = nullptr;
        struct sockaddr_storage peer_addr = {0};
        socklen_t peer_addr_len = 0;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
        hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
        hints.ai_protocol = 0;          /* Any protocol */
        hints.ai_canonname = nullptr;
        hints.ai_addr = nullptr;
        hints.ai_next = nullptr;

        if(const auto success= getaddrinfo(nullptr, port.data(), &hints, &result); success!=0){
            throw SocketError(fmt::format("listenAndBind getaddrinfo failed: {}", gai_strerror(success)));
        }

        for (rp = result; rp != nullptr; rp = rp->ai_next) {
            m_socket = socket(rp->ai_family, rp->ai_socktype,
                         rp->ai_protocol);
            if (m_socket == -1)
                continue;

            // allow socket to be reused
            const int on = 1;
            if(setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,(char*)&on, sizeof(on)) < 0) {
                fmt::print("could not set socket {} to reuse address: {}", m_socket, strerror(errno));
                ::close(m_socket);
                continue;
            }
            if(t_non_blocking) {
                if(::ioctl(m_socket, FIONBIO, (char*)&on) < 0) {
                    fmt::print("could not set socket {} to non blocking: {}", m_socket, strerror(errno));
                    ::close(m_socket);
                    continue;
                }
            }

            if (bind(m_socket, rp->ai_addr, rp->ai_addrlen) == 0)
                break;                  /* Success */

            ::close(m_socket);
        }
        freeaddrinfo(result);

        if(listen(m_socket, t_backlog) == -1) {
            ::close(m_socket);
            throw SocketError(fmt::format("could not listen on socket {}: {}", m_socket, strerror(errno)));
        }
        setIsOpen(true);
    }

    void ServerSocket::setAcceptCallback(ServerSocket::AcceptCallback const &accept_callback) {
        m_accept_callback = accept_callback;
    }

    ServerSocket::~ServerSocket() {
        std::lock_guard lock{m_mutex};
        if(isOpen()) {
            close();
        }
    }

    std::unique_ptr<ClientSocket> ServerSocket::accept(std::chrono::milliseconds t_timeout) {
        if(!isOpen()) {
            throw SocketError(fmt::format("socket not open {}\n", m_socket));
        }
        pollfd fds[1];
        fds[0].fd = m_socket;
        fds[0].events = POLLIN;

        if(const auto ret = poll(fds, 1, static_cast<int>(t_timeout.count()));ret == 0) {
            throw SocketTimeoutError(fmt::format("waiting for incoming connection failed on socket {}: {}\n", m_socket, strerror(errno)));
        } else if(ret < 0) {
            throw SocketError(fmt::format("waiting for incoming connection poll error on socket {}: {}\n", m_socket,
                                          strerror(errno)));
        }

        sockaddr_in client = {0};
        socklen_t client_addrLen = sizeof(client);

        int clientSocket = ::accept(m_socket, reinterpret_cast<sockaddr*>(&client), &client_addrLen);
        if (clientSocket == -1) {
            throw SocketError(fmt::format("could not accept incoming connection on socket {}: {}\n", m_socket, strerror(errno)));
        }
        return std::make_unique<ClientSocket>(clientSocket, Peer{inet_ntoa(client.sin_addr),
                                               ntohs(client.sin_port)}, true);

    }


}