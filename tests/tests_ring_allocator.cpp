
#include "y_ring_allocator.hpp"
#include "catch.hpp"

TEST_CASE("Construction", "[RingAllocator]") {
    uint8_t buffer [10'000];
    y::RingAllocator ra (buffer, sizeof(buffer));

    REQUIRE(ra.capacity() == sizeof(buffer));
}
