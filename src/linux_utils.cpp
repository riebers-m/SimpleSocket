//
// Created by max on 03.01.24.
//
#include "internal/simple_types.hpp"

namespace simple {
    int convert_address_family(address_family t_address_family) {
        switch (t_address_family) {
            case address_family::INET:
                return AF_INET;
            case address_family::INET6:
                return AF_INET6;
            case address_family::LOCAL:
                return AF_LOCAL;
            case address_family::PACKET :
            case address_family::UNIX :
                return AF_UNIX;
            case address_family::UNSPEC:
                return AF_UNSPEC;
            default:
                return -1;
        }
    }
    int convert_socket_type(socket_type t_socket_type) {
        switch (t_socket_type) {
            case socket_type::TCP:
                return SOCK_STREAM;
            case socket_type::UDP:
                return SOCK_DGRAM;
            default:
                return -1;
        }
    }
}