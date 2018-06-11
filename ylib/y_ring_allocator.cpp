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


