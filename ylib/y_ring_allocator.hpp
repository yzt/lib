#pragma once

#if !defined(Y_RING_ALLOCATOR_H_INCLUDE_GUARD_)
    #define  Y_RING_ALLOCATOR_H_INCLUDE_GUARD_

#if 0
#include <cstddef> // for size_t
#include <cstdint>

namespace y {

struct Buffer {
    uint8_t * ptr;
    uint32_t capacity;
};

class RingAllocator {
    enum class State : uint8_t {
        INVALID = 0,
        Free = 1,
        Allocated = 2,
        Padding = 3,
    };

    struct Header {
        uint32_t size;  // Stores total size, including header and footer
        State state;
        char sentinel [3];
    };
    static_assert(sizeof(Header) == 8, "");

    struct Footer {
        char sentinel [3];
        uint8_t is_free;
        uint32_t size;  // Stores total size, including header and footer
    };
    static_assert(sizeof(Footer) == 8, "");

public:
    RingAllocator (void * buffer, uint32_t size);
    ~RingAllocator ();

    uint32_t capacity () const {return uint32_t(m_capacity);}
    uint32_t size_allocated () const {return uint32_t(m_write_pos - m_read_pos);}
    uint32_t size_available () const {return capacity() - size_allocated();}
    bool empty () const {return 0 == size_allocated();}

    Buffer alloc (uint32_t size);
    bool free (Buffer buffer);

private:
    uint8_t * m_mem = nullptr;
    uint32_t const m_capacity = 0;
    uint64_t m_read_pos = 0;
    uint64_t m_write_pos = 0;
};

}
#endif

#include <stddef.h> // for size_t
#include <stdint.h> // for uint8_t, etc.

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint32_t allocator_size_t;

typedef struct {
    uint8_t * ptr;
    allocator_size_t size;
} allocator_block_t;

typedef struct {
    void * mem;
    void * free;
    uint64_t capacity;
    uint64_t remaining;
} allocator_ring_t;


bool
Allocator_Ring_Init (
    allocator_ring_t * out_allocator,
    allocator_block_t backing_memory
);

bool
Allocator_Ring_Cleanup (
    allocator_ring_t * allocator
);

allocator_block_t
Allocator_Ring_Alloc (
    allocator_size_t size
);

bool
Allocator_Ring_Free (
    allocator_block_t mem
);

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_RING_ALLOCATOR_H_INCLUDE_GUARD_
