//
// Created by max on 03.01.24.
//
#include <stdexcept>
#include <string>

#ifndef SIMPLESOCKET_SIMPLE_TYPES_HPP
#define SIMPLESOCKET_SIMPLE_TYPES_HPP
#if __linux__

#include <sys/types.h>
#include <sys/socket.h>
#include "linux_details.hpp"
#elif _WIN32
#error "Platform not implemented"
#else
#error "Platform not implemented"
#endif

namespace simple {
#if __linux__
    using socket_t = int;
#elif _WIN32
    #error "Platform not implemented"
#else
#error "Platform not implemented"
#endif

enum class address_family {
    UNIX,
    LOCAL,
    INET,
    INET6,
    PACKET,
    UNSPEC, // allow ipv4 or ipv6
};
enum class socket_type {
    TCP,
    UDP,
};

int convert_address_family(address_family);
int convert_socket_type(socket_type);


}
#endif //SIMPLESOCKET_SIMPLE_TYPES_HPP
