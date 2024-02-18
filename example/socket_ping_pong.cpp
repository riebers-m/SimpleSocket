#include "simple_sockets.hpp"
#include <fmt/color.h>
#include <thread>
#include <chrono>

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
        std::jthread{[this]() {
            auto client_callback = [](std::vector<char> const& request)->std::vector<char> {
                fmt::println("received: {}", std::string{request.cbegin(), request.cend()});
                const char* response = "pong";
                return std::vector<char>{response, response+4};
            };
            while(m_server_socket.is_open()) {
                try {
                    auto client = m_server_socket.accept(client_callback);
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
private:

    std::vector<simple::Client> m_clients;
    simple::ServerSocket m_server_socket;
};

void run_server() {
    auto server = Server{12345};

    std::this_thread::sleep_for(10s);
    fmt::println("shutting server down");
    server.stop();
}

int main() {
    run_server();
    return 0;
}