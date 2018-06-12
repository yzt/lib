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
BlockOverHead () {
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

static inline uint32_t
GetOccupiedFromHeader (uint8_t const * ptr) {
    auto p = reinterpret_cast<Header const *>(ptr);
    return 0 != (p->size_and_occupied & 1);
}

static inline void
PutFooterAt (uint8_t * ptr, allocator_size_t user_size, bool occupied) {
    return PutHeaderAt(ptr, user_size, occupied);
}

static inline uint32_t
GetSizeFromFooter (uint8_t const * ptr) {
    return GetSizeFromHeader(ptr);
}

static inline uint32_t
GetOccupiedFromFooter (uint8_t const * ptr) {
    return GetOccupiedFromHeader(ptr);
}

static inline void
PutBlockAt (uint8_t * ptr, allocator_size_t total_size, bool mark_as_occupied, bool zero_out) {
    assert(ptr);
    assert(total_size >= sizeof(Header) + sizeof(Footer));
    auto user_size = total_size - BlockOverHead();
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
    allocator_block_t backing_memory
) {
    bool ret = false;
    if (out_allocator && backing_memory.ptr && backing_memory.size >= sizeof(Header) + sizeof(Footer)) {
        *out_allocator = {};
        out_allocator->mem = Align<2>(backing_memory.ptr);
        out_allocator->free = out_allocator->mem;
        out_allocator->capacity = allocator_size_t(backing_memory.ptr + backing_memory.size - out_allocator->mem);
        out_allocator->remaining = out_allocator->capacity - BlockOverHead();
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

    return ret;
}

allocator_block_t
Allocator_Ring_Alloc (
    allocator_size_t size
) {
    allocator_block_t ret = {};

    return ret;
}

bool
Allocator_Ring_Free (
    allocator_block_t mem
) {
    bool ret = false;

    return ret;
}
