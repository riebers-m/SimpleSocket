#include "simple_socket.hpp"
#include <fmt/color.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main(int argc, const char **argv) {
    try {
        bool running = true;
        auto sock = simple::TcpSocket::connect("localhost", "5000");
        sock.setReceiveCallback([&running](std::vector<char> const & data) {
            fmt::print("recieved data: {}", data.data());
            std::string_view view{data};
            if(view.contains("quit")) {
                fmt::print("main thread shutting down\n");
                running = false;
            }
        });
        sock.run();

        while(running) {
            sock.send("hello there\n");

            std::this_thread::sleep_for(2s);

        }
    } catch (simple::SocketError const &e) {
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "{}", e.what());
    }

    return 0;
}