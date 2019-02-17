
#include "../experimental/y_ring_allocator.hpp"
#include "catch.hpp"

#if 0
TEST_CASE("RingAllocator Construction", "[alloc]") {
    uint8_t buffer [10'000];

    allocator_ring_t ra;
    REQUIRE(Allocator_Ring_Init(&ra, {buffer, sizeof(buffer)}, nullptr));

    REQUIRE(ra.capacity == sizeof(buffer));

    auto f = [] (allocator_ring_t *, int block_num, uint8_t * ptr, allocator_size_t size, bool occupied, uint8_t invalidity_bits, void *) -> bool {
        CHECK(ptr != nullptr);
        CHECK(0 == invalidity_bits);
        return true;
    };

    auto block_count_0 = Allocator_Ring_WalkBlocks(&ra, nullptr, f);
    CHECK(1 == block_count_0);

    auto block1 = Allocator_Ring_Alloc(&ra, 80);
    CHECK(block1.ptr != nullptr);
    CHECK(block1.size == 80);

    auto block_count_1 = Allocator_Ring_WalkBlocks(&ra, nullptr, f);
    CHECK(2 == block_count_1);

    auto free_1 = Allocator_Ring_Free(&ra, block1);
    CHECK(free_1);

    auto block_count_2 = Allocator_Ring_WalkBlocks(&ra, nullptr, f);
    CHECK(1 == block_count_2);

    REQUIRE(Allocator_Ring_Cleanup(&ra));
}
#endif
