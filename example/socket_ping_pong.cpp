#include "simple_sockets.hpp"
#include <fmt/base.h>
#include <fmt/color.h>
#include <thread>
#include <chrono>
#include <stdint.h>

using namespace std::chrono_literals;

simple::Client &&func(simple::Client &&client) {
    auto const [host, port] = client.getPeer();
    fmt::print("connected with {}:{}", host, port);
    client.send("hello there");
    std::this_thread::sleep_for(5s);
    fmt::print("leavin func\n\n");
    return std::move(client);
}

void run_client() {
    auto client = simple::Sockets::create_client("localhost", 12345,
                                                 [](std::vector<char> const &message) -> std::vector<char> {
                                                     fmt::print("received:\n\n{}",
                                                                std::string{message.cbegin(), message.cend()});
                                                     return {};
                                                 });
    try {
        auto client2 = func(std::move(client));
        while (client2.is_open()) {
            std::this_thread::sleep_for(1s);
        }

    } catch (simple::SocketError const &e) {
        fmt::print("{}", e.what());
    }
}


class Server {
public:
    explicit Server(std::uint16_t port) :
    m_clients(),
    m_server_socket(simple::Sockets::create_server(port, simple::ServerSocket::blocking::blocking)) {
        std::jthread{[this,&port]() {
            auto client_callback = [](std::vector<char> const& request)->std::vector<char> {
                fmt::println("received: {}", std::string{request.cbegin(), request.cend()});
                const char* response = "pong";
                return std::vector<char>{response, response+4};
            };
            while(m_server_socket.is_open()) {
                try {
                    auto client = m_server_socket.accept(client_callback);
                    fmt::println("connected with client: {}", client.getPeer().host);
                    m_clients.push_back(std::move(client));
                } catch(simple::SocketTimeoutError const&) {
                    continue;
                } catch(simple::SocketError const& e) {
                    fmt::println("{}", e.what());
                }
            }
        }}.detach();
    }

    void stop() {
        m_server_socket.close();
    }

    ~Server() {
        for(auto&& client : m_clients) {
            client.close();
        }
    }
private:

    std::vector<simple::Client> m_clients;
    simple::ServerSocket m_server_socket;
};

void run_server(std::uint32_t port, std::string address) {
    auto constexpr port = 1234;
    fmt::print("Starting server on port: {}", port);
    auto server = Server{port};

    std::this_thread::sleep_for(100s);
    fmt::println("shutting server down");
    server.stop();
}

int main(int argc, const char* argv[]) {
    if(argc < 3) {
        fmt::println("usage: Server address and port for listening needed.");
        fmt::println("E.g.: ./socket_ping_pong <address> <port>");
        return -1;
    }
    run_server();
    return 0;
}