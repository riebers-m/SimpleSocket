//
// Created by max on 05.01.24.
//
#include "simple_socket.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <thread>

namespace simple {
    ServerSocket ServerSocket::bindAndListen(std::string_view port, int t_backlog) {
        struct addrinfo hints = {0};
        struct addrinfo *result, *rp = nullptr;
        socket_t sfd, s = 0;
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

        s = getaddrinfo(nullptr, port.data(), &hints, &result);
        if (s != 0) {
            throw SocketError(fmt::format("listenAndBind getaddrinfo failed: {}", gai_strerror(s)));
        }

        for (rp = result; rp != nullptr; rp = rp->ai_next) {
            sfd = socket(rp->ai_family, rp->ai_socktype,
                         rp->ai_protocol);
            if (sfd == -1)
                continue;

            if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
                break;                  /* Success */

            ::close(sfd);
        }
        freeaddrinfo(result);

        if(listen(sfd, t_backlog) == -1) {
            ::close(sfd);
            throw SocketError(fmt::format("could not listen on socket {}: {}", sfd, strerror(errno)));
        }
        return ServerSocket{sfd};
    }

    ServerSocket::ServerSocket(socket_t t_sock) : TcpSocket(t_sock, {}){

    }

    void ServerSocket::setAcceptCallback(ServerSocket::AcceptCallback const &accept_callback) {
        m_accept_callback = accept_callback;
    }

    ServerSocket::~ServerSocket() {
        std::lock_guard lock{m_mutex};
        close();
    }

    ClientSocket ServerSocket::accept() {

        sockaddr_in client = {0};
        socklen_t client_addrLen = sizeof(client);

        int clientSocket = ::accept(m_socket, reinterpret_cast<sockaddr*>(&client), &client_addrLen);
        if (clientSocket == -1) {
            throw SocketError(fmt::format("could not accept incoming connection on socket {}: {}", m_socket, strerror(errno)));
        }
        return ClientSocket{clientSocket, Peer{inet_ntoa(client.sin_addr),
                                               std::to_string(ntohs(client.sin_port))}};

    }
}