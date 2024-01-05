#include "simple_socket.hpp"
#include <fmt/color.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

void run_client() {
    try {
        bool running = true;
        auto sock = simple::ClientSocket::connect("localhost", "5000");
        sock.setReceiveCallback([&running](std::vector<char> const & data) -> std::vector<char> const {
            fmt::print("recieved data: {}", data.data());
            std::string_view view{data};
            if(view.contains("quit")) {
                fmt::print("main thread shutting down\n");
                running = false;
            }
            return std::vector<char>{};
        });
        sock.run();

        while(running) {
            sock.send("hello there\n");

            std::this_thread::sleep_for(2s);

        }
    } catch (simple::SocketError const &e) {
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "{}", e.what());
    }
}

void run_server_sock() {
    auto sock = simple::ServerSocket::bindAndListen("5000");
    auto new_sock = sock.accept();

    fmt::print("accepted new connection from {}:{}\n", new_sock.getPeer().host, new_sock.getPeer().service);

    bool running = true;
    new_sock.setReceiveCallback([&running](std::vector<char> const & data) -> std::vector<char> {
        fmt::print("recieved data: {}", data.data());
        std::string_view view{data};
        if(view.contains("quit")) {
            fmt::print("main thread shutting down\n");
            running = false;
            return {};
        } else {
            return data;
        }
    });
    new_sock.run();

    while(running) {
        std::this_thread::sleep_for(2s);
    }
    new_sock.stop();
}
int main(int argc, const char **argv) {
    run_server_sock();

    return 0;
}