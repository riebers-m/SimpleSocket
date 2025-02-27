//
// Created by max on 16.02.24.
//
#include <utility>
#include "simple_socket.hpp"
#include <fmt/format.h>
#include <fmt/color.h>

namespace simple {
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 2048;

    static void socket_deleter(socket_t socket) {
        if (::close(socket) == -1) {
            fmt::print("closing socket failed: {}", strerror(errno));
        }
    }

    BaseSocket::BaseSocket(socket_t socket, bool is_open) : m_socket{socket, socket_deleter}, m_is_open{is_open} {

    }

    BaseSocket::BaseSocket(socket_t socket, BaseSocket::unique_deleter deleter, bool is_open) : m_socket{socket, deleter},
                                                                                  m_is_open{is_open} {

    }

    static socket_t initialize_and_connect(std::string const &host, std::uint16_t port);

    Client::Client(socket_t socket, ReceiveCallback callback, Peer const &peer) :
            BaseSocket(socket, true),
            m_peer{peer},
            m_callback{std::move(callback)},
            m_mutex() {
        m_worker = std::jthread{std::bind_front(&Client::waiting_for_incoming_message, this)};
    }

    Client::Client(std::string const &host, std::uint16_t port, ReceiveCallback callback)
            : Client{initialize_and_connect(host, port), std::move(callback), Peer{host, port}} {
    }

    Client::Client(socket_t sock, Client::ReceiveCallback const &callback) :
            Client(sock, callback, {}) { }

    socket_t initialize_and_connect(std::string const &host, std::uint16_t port) {
        struct addrinfo hints = {0};
        struct addrinfo *result, *rp = nullptr;
        socket_t sock{};

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          /* Any protocol */

        if (auto ret = getaddrinfo(host.data(), std::to_string(port).c_str(), &hints, &result);ret != 0) {
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
        return sock;
    }

    std::vector<char> Client::receive() const {
        if (!m_is_open) {
            throw SocketError(fmt::format("socket not open"));
        }
        std::size_t read{0};
        char tmp_buffer[DEFAULT_BUFFER_SIZE] = {0};
        {
            std::lock_guard lock{m_mutex};
            read = recv(m_socket.value(), tmp_buffer, DEFAULT_BUFFER_SIZE, 0);
        }
        if (read == 0) {
            throw SocketShutdownError(fmt::format("peer has shutdown connection on socket {}", m_socket.value()));
        }
        if (read == -1) {
            throw SocketError(fmt::format("reading from socket failed: {}", strerror(errno)));
        }
        return std::vector<char>{tmp_buffer, tmp_buffer + read};
    }

    std::size_t Client::send(std::string_view const message) {
        return send(std::vector<char>{message.begin(), message.end()});
    }

    std::string Client::receive_string() const {
        auto const received_bytes = receive();
        return std::string{received_bytes.cbegin(), received_bytes.cend()};
    }

    std::size_t Client::send(std::vector<char> const &message) {
        if (message.empty()) { throw SocketError(fmt::format("empty send buffer")); }
        if (!m_is_open) {
            throw SocketShutdownError(fmt::format("socket not open"));
        }

        std::size_t sent_bytes = 0;
        const char *send_ptr = message.data();
        auto tries = 3;

        std::lock_guard lock{m_mutex};
        while (sent_bytes < message.size() && tries > 0) {
            std::size_t sent = 0;
            if (sent = ::send(m_socket.value(), send_ptr + sent_bytes, message.size() - sent_bytes, 0); sent == -1) {
                throw SocketError(fmt::format("could not send message: {}", strerror(errno)));
            }
            sent_bytes += sent;
            tries--;
        }
        return sent_bytes;
    }

    void Client::waiting_for_incoming_message(std::stop_token const &stop_token) {
        if (!m_callback) {
            fmt::print("empty callback shutting down");
            return;
        }

        pollfd fds[1];
        fds[0].fd = m_socket.value();
        fds[0].events = POLLIN;

        while (!stop_token.stop_requested()) {
            if (!m_is_open) {
                fmt::print("socket {} not open", m_socket.value());
                return;
            }
            auto result = 0;
            if (result = poll(fds, 1, 10);result == -1) {
                fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "poll error: {}", strerror(errno));
                return;
            } else if (result == 0) {
                continue;
            }

            if (m_callback) {
                try {
                    if (const auto response = m_callback(receive());!response.empty()) {
                        send(response);
                    }
                } catch (SocketShutdownError const &e) {
                    fmt::println("{}", e.what());
                    m_is_open = false;
                    return;
                } catch (SocketError const &e) {
                    fmt::println("communication error on socket {}: {}", m_socket.value(), e.what());
                } catch(std::bad_function_call const& e) {

                }
            }
        }
        fmt::println("thread shutting down");
    }

    // https://stackoverflow.com/questions/29986208/how-should-i-deal-with-mutexes-in-movable-types-in-c
    Client::Client(Client &&other) noexcept: BaseSocket(other.m_socket.value(), [](socket_t) {}) {
        {
            std::unique_lock lock{other.m_mutex};
            m_is_open = other.m_is_open;
            m_callback = std::move(other.m_callback);
            m_socket = std::move(other.m_socket);
            m_worker = std::jthread(std::bind_front(&Client::waiting_for_incoming_message, this));
        }
        other.close();
    }

    Client &Client::operator=(Client &&other) noexcept {
        if (this != std::addressof(other)) {
            {
                std::unique_lock lock{other.m_mutex};
                m_is_open = other.m_is_open;
                m_callback = std::move(other.m_callback);
                m_socket = std::move(other.m_socket);
                m_worker = std::jthread(std::bind_front(&Client::waiting_for_incoming_message, this));
            }
            other.close();
        }
        return *this;
    }

    Client::~Client() {
        close();
    }

    bool Client::is_open() const {
        return m_is_open;
    }

    void Client::close() {
        std::unique_lock lock{m_mutex};
        m_is_open = false;
        m_worker.request_stop();
        if (m_worker.joinable()) {
            m_worker.join();
        }
    }

    Peer const &Client::getPeer() const {
        return m_peer;
    }

    socket_t initialize_bind_and_listen(std::uint16_t port, simple::ServerSocket::blocking blocking) {
        struct addrinfo hints = {0};
        struct addrinfo *result, *rp = nullptr;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
        hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
        hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
        hints.ai_protocol = 0;          /* Any protocol */
        hints.ai_canonname = nullptr;
        hints.ai_addr = nullptr;
        hints.ai_next = nullptr;

        if(const auto success= getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result); success!=0){
            throw SocketError(fmt::format("listenAndBind getaddrinfo failed: {}", gai_strerror(success)));
        }
        socket_t sock = 0;
        for (rp = result; rp != nullptr; rp = rp->ai_next) {
            sock = socket(rp->ai_family, rp->ai_socktype,
                              rp->ai_protocol);
            if (sock == -1)
                continue;

            // allow socket to be reused
            const int on = 1;
            if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,(char*)&on, sizeof(on)) < 0) {
                fmt::print("could not set socket {} to reuse address: {}", sock, strerror(errno));
                ::close(sock);
                continue;
            }
            if(blocking == ServerSocket::blocking::not_blocking) {
                if(::ioctl(sock, FIONBIO, (char*)&on) < 0) {
                    fmt::print("could not set socket {} to non blocking: {}", sock, strerror(errno));
                    ::close(sock);
                    continue;
                }
            }

            if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
                break;                  /* Success */
            }
            ::close(sock);
        }
        freeaddrinfo(result);

        if(listen(sock, 0) == -1) {
            ::close(sock);
            throw SocketError(fmt::format("could not listen on socket {}: {}", sock, strerror(errno)));
        }
        return sock;
    }

    ServerSocket::ServerSocket(std::uint16_t port, blocking accept_blocking, std::chrono::milliseconds const &accept_timeout) :
            BaseSocket{initialize_bind_and_listen(port, accept_blocking), true},
            m_accept_timeout{accept_timeout}{ }

    bool ServerSocket::is_open() const {
        return m_is_open;
    }

    Client ServerSocket::accept(Client::ReceiveCallback const& callback) {
        if(!m_is_open) {
            throw SocketError(fmt::format("socket not open {}\n", m_socket.value()));
        }
        pollfd fds[1];
        fds[0].fd = m_socket.value();
        fds[0].events = POLLIN;

        if(const auto ret = poll(fds, 1, static_cast<int>(m_accept_timeout.count()));ret == 0) {
            throw SocketTimeoutError(fmt::format("waiting for incoming connection failed on socket {}: {}\n", m_socket.value(), strerror(errno)));
        } else if(ret < 0) {
            throw SocketError(fmt::format("waiting for incoming connection poll error on socket {}: {}\n", m_socket.value(),
                                          strerror(errno)));
        }

        sockaddr_in client = {0};
        socklen_t client_addrLen = sizeof(client);

        socket_t const clientSocket = ::accept(m_socket.value(), reinterpret_cast<sockaddr*>(&client), &client_addrLen);
        if (clientSocket == -1) {
            throw SocketError(fmt::format("could not accept incoming connection on socket {}: {}\n", m_socket.value(), strerror(errno)));
        }
        return Client{clientSocket, callback, Peer{inet_ntoa(client.sin_addr),ntohs(client.sin_port)}};
    }

    void ServerSocket::close() {
        m_is_open = false;
    }
}

