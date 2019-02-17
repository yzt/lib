
#include "../experimental/y_socket.hpp"
#include "catch.hpp"

TEST_CASE("Basics of Socket 1", "[socket]") {
    y::Socket::LibInitializer sock_lib;

    y::Socket::UDP::Config cfg;
    cfg.recv_buffer_size = 64 * 1024;
    cfg.bind_address = y::Socket::Address::IPv4Any(0);
    y::Socket::UDP udp {cfg};

    REQUIRE_FALSE(udp.hasMsg());
}
