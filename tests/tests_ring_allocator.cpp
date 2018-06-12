
#include "y_ring_allocator.hpp"
#include "catch.hpp"

TEST_CASE("Construction", "[RingAllocator]") {
    uint8_t buffer [10'000];

    allocator_ring_t ra;
    Allocator_Ring_Init(&ra, {buffer, sizeof(buffer)});

    REQUIRE(ra.capacity == sizeof(buffer));

    Allocator_Ring_Cleanup(&ra);
}
