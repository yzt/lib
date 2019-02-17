#if 0
#include "y_ring_allocator.hpp"
#include <cassert>

namespace y {

RingAllocator::RingAllocator (void * buffer, uint32_t size)
    : m_mem ((uint8_t *)buffer)
    , m_capacity (size)
{
}

RingAllocator::~RingAllocator () {
}

Buffer RingAllocator::alloc (uint32_t size) {
    Buffer ret = {};

    uint32_t effective_size = 4 + size;
    uint32_t available_size = uint32_t(static_cast<uint64_t>(m_write_pos - m_read_pos));
    if (effective_size <= available_size) {
        //uint32_t effective_write_pos = uint32_t(m_write_pos % m_capacity);
        //uint32_t effective_read_pos = uint32_t(m_read_pos % m_capacity);

        //if (effective_write_pos
    }
    return ret;
}

bool RingAllocator::free (Buffer buffer) {
    return false;
}

}
#endif

#include "y_ring_allocator.hpp"
#include <cassert>
#include <cstring>  // memset()

struct Header {
    allocator_size_t size_and_occupied;
};

typedef Header Footer;

static_assert(sizeof(Header) == 4, "");
static_assert(sizeof(Footer) == 4, "");

static inline allocator_size_t
BlockOverhead () {
    return allocator_size_t(sizeof(Header) + sizeof(Footer));
}

static inline void
PutHeaderAt (uint8_t * ptr, allocator_size_t user_size, bool occupied) {
    assert((user_size & 1) == 0);
    auto p = reinterpret_cast<Header *>(ptr);
    p->size_and_occupied = user_size | (occupied ? 1 : 0);
}

static inline uint32_t
GetSizeFromHeader (uint8_t const * ptr) {
    auto p = reinterpret_cast<Header const *>(ptr);
    return p->size_and_occupied & ~allocator_size_t(1);
}

static inline bool
GetOccupiedFromHeader (uint8_t const * ptr) {
    auto p = reinterpret_cast<Header const *>(ptr);
    return 0 != (p->size_and_occupied & 1);
}

static inline uint8_t const *
GetFooterAddressFromHeader (uint8_t const * ptr) {
    return ptr + sizeof(Header) + GetSizeFromHeader(ptr);
}

static inline uint8_t *
GetFooterAddressFromHeader (uint8_t * ptr) {
    return ptr + sizeof(Header) + GetSizeFromHeader(ptr);
}

static inline void
PutFooterAt (uint8_t * ptr, allocator_size_t user_size, bool occupied) {
    return PutHeaderAt(ptr, user_size, occupied);
}

static inline uint32_t
GetSizeFromFooter (uint8_t const * ptr) {
    return GetSizeFromHeader(ptr);
}

static inline bool
GetOccupiedFromFooter (uint8_t const * ptr) {
    return GetOccupiedFromHeader(ptr);
}

static inline uint8_t const *
GetHeaderAddressFromFooter (uint8_t const * ptr) {
    return ptr - GetSizeFromFooter(ptr) - sizeof(Header);
}

static inline uint8_t *
GetHeaderAddressFromFooter (uint8_t * ptr) {
    return ptr - GetSizeFromFooter(ptr) - sizeof(Header);
}

static inline void
PutBlockAt (uint8_t * ptr, allocator_size_t total_size, bool mark_as_occupied, bool zero_out) {
    assert(ptr);
    assert(total_size >= sizeof(Header) + sizeof(Footer));
    auto user_size = total_size - BlockOverhead();
    PutFooterAt(ptr + total_size - sizeof(Footer), user_size, mark_as_occupied);
    PutHeaderAt(ptr, user_size, mark_as_occupied);
    if (zero_out)
        ::memset(ptr + sizeof(Header), 0, user_size);
}

template <size_t AlignTo, typename T>
static inline T
Align (T p) {
    return (p + AlignTo - 1) / AlignTo * AlignTo;
}

template <size_t AlignTo, typename T>
static inline T *
Align (T * p) {
    return reinterpret_cast<T *>(uintptr_t(p + AlignTo - 1) / AlignTo * AlignTo);
}

bool
Allocator_Ring_Init (
    allocator_ring_t * out_allocator,
    allocator_block_t backing_memory,
    void * user_data
) {
    bool ret = false;
    if (out_allocator && backing_memory.ptr && backing_memory.size >= sizeof(Header) + sizeof(Footer)) {
        *out_allocator = {};
        out_allocator->mem = Align<2>(backing_memory.ptr);
        out_allocator->free = out_allocator->mem;
        out_allocator->capacity = allocator_size_t(backing_memory.ptr + backing_memory.size - out_allocator->mem);
        out_allocator->remaining = out_allocator->capacity - BlockOverhead();
        out_allocator->user_data = user_data;
        PutBlockAt(out_allocator->mem, out_allocator->capacity, false, false);
        ret = true;
    }
    return ret;
}

bool
Allocator_Ring_Cleanup (
    allocator_ring_t * allocator
) {
    bool ret = false;
    if (allocator) {
        *allocator = {};
        ret = true;
    }
    return ret;
}

int Allocator_Ring_WalkBlocks (
    allocator_ring_t * allocator,
    void * walk_user_data,
    allocator_ring_block_walk_f walk_cb
) {
    int ret = -1;
    if (allocator && allocator->mem && walk_cb) {
        ret = 0;
        auto p = allocator->mem;
        auto end = allocator->mem + allocator->capacity;
        while (p + BlockOverhead() < end) {
            auto block_size_hdr = GetSizeFromHeader(p);
            auto occupied_hdr = GetOccupiedFromHeader(p);
            auto q = GetFooterAddressFromHeader(p);
            auto p2 = GetHeaderAddressFromFooter(q);
            auto block_size_ftr = GetSizeFromFooter(q);
            auto occupied_ftr = GetOccupiedFromFooter(q);

            uint8_t invalidity_bits = 0
                | (block_size_hdr == block_size_ftr ? 0 : 1)    // bit 0: size mismatch from header and footer
                | (occupied_hdr == occupied_ftr ? 0 : 2)        // bit 1: occupancy mismatch from header and footer
                | (p == p2 ? 0 : 4)                             // bit 2: another check on size, basically
                ;

            bool cb_ret = walk_cb(allocator, ret, p + sizeof(Header), block_size_hdr, occupied_hdr, invalidity_bits, walk_user_data);

            ret += 1;
            if (!cb_ret)
                break;

            p = q + sizeof(Footer);
        }
    }
    return ret;
}

allocator_block_t
Allocator_Ring_Alloc (
    allocator_ring_t * allocator,
    allocator_size_t size
) {
    allocator_block_t ret = {};

    return ret;
}

bool
Allocator_Ring_Free (
    allocator_ring_t * allocator,
    allocator_block_t mem
) {
    bool ret = false;

    return ret;
}
