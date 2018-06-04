#pragma once

//======================================================================

#include <atomic>
#include <cassert>
#include <cstdint>
#include <optional> // std::optional
#include <utility>  // std::move()

//======================================================================

#if !defined(Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR)
#define Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR  0
#endif

//======================================================================

#define Y_ASSERT(cond, ...)         assert(cond)
#define Y_ASSERT_STRONG(cond, ...)  assert(cond)

namespace y {

using u16 = uint16_t;
using u64 = uint64_t;

template <typename T>
using Optional = std::optional<T>;

}

//======================================================================

namespace y {
    namespace Lockfree {
	
//======================================================================
//======================================================================
//----------------------------------------------------------------------
//======================================================================

template <typename T>
class Queue {
public:
    Queue (T * buffer, unsigned max_count)
        : m_data (buffer)
        , m_capacity (max_count)
        , m_cursors (0)
    {
        Y_ASSERT_STRONG(nullptr != m_data);
        Y_ASSERT_STRONG(m_capacity > 0);
        Y_ASSERT_STRONG((m_capacity & (m_capacity - 1)) == 0, "Capacity must be a power of two (so when the 32-bit cursors overflow, it won't break this implementation.)");
        Y_ASSERT_STRONG(m_capacity < (1 << 16), "Definitely must be smaller than 64K (not equal; strictly smaller).");
    }

    ~Queue () noexcept {
        auto cursors = Unpack(m_cursors.load());
        Y_ASSERT_STRONG(cursors.readable == cursors.reading, "Should not be reading the queue while destroying!");
        Y_ASSERT_STRONG(cursors.writable == cursors.writing, "Should not be writing the queue while destroying!");
        for (unsigned i = cursors.readable, e = cursors.writing; i != e; ++i)
            (m_data + (i % m_capacity))->T::~T();
    }

    /* The four "cursors" are indexes into the queue (assume they don't wrap around, for the moment)
    *   reading: the first item that is being read (i.e. on the way to be removed from the queue)
    *   readable: the first item that can be read (the next "get" happens here)
    *   writing: the first item that is being written to right now (i.e. on the way to be added to the queue)
    *   writable: the first really free index in the queue (the next "put" happens here)
    * The four cursors, barring wrap around, have the following relationships:
    *   reading <= readable <= writing <= writable
    * And:
    *   readable - reading: the number of items being read at this instant
    *   writing - readable: the number of complete items that are not being read from or written to (I call this the "inner size".)
    *   writable - writing: the number of items being written to right now
    *   writable - reading: the "outer size", the total items that are being occupied right now
    *   writable - readable: the "size"; the sum of the complete and untouched items plus items being written; if you don't touch the queue for a short time, this will become the inner size (after the writes finish)
    * The wrap-around works like this:
    *   Since each cursor is 16 bits in size, and the capacity is a power
    *   of two strictly less than 2**16, the subtractions should work as
    *   expected.
    */
    unsigned size () const noexcept {
        auto cursors = Unpack(m_cursors.load());
        return unsigned(cursors.writable - cursors.readable);
    }
    unsigned size_inner () const noexcept {
        auto cursors = Unpack(m_cursors.load());
        return unsigned(cursors.writing - cursors.readable);
    }
    unsigned size_outer () const noexcept {
        auto cursors = Unpack(m_cursors.load());
        return unsigned(cursors.writable - cursors.reading);
    }
    bool empty () const noexcept {
        return size_inner() <= 0;
    }
    bool full () const noexcept {
        return size_outer() >= m_capacity;
    }
    unsigned capacity () const noexcept {
        return m_capacity;
    }

    bool put (T v) const noexcept {
        u16 insert_cursor;
        u64 new_cursors;
        u64 cur_cursors;
        Cursors cursors;

        // Reserve a "writing" slot, if available...
        cur_cursors = m_cursors.load();
        do {
            cursors = Unpack(cur_cursors);
            // Outer size must be less than capacity
            if (u16(cursors.writable - cursors.reading) >= m_capacity)
                return false;
            insert_cursor = cursors.writable;
            cursors.writable += 1;
            new_cursors = Pack(cursors);
        } while (!std::atomic_compare_exchange_strong(&m_cursors, &cur_cursors, new_cursors));
        
        // Do the actual writing...
        new (m_data + (insert_cursor % m_capacity)) T (std::move(v));

        // Spin till the writes that started before this one have finished...
    #if Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 0
        do {
            cursors = Unpack(m_cursors.load());
        } while (cursors.writing != insert_cursor);
    #elif Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 1
        for (;;) {
            cursors = Unpack(m_cursors.load());
            if (cursors.writing == insert_cursor)
                break;
            std::this_thread::yield();
        }
    #elif Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 2
        for (int i = 0; i < 100; ++i) {
            cursors = Unpack(m_cursors.load());
            if (cursors.writing == insert_cursor)
                break;
        }
        if (cursors.writing != insert_cursor)
            for (;;) {
                std::this_thread::yield();
                cursors = Unpack(m_cursors.load());
                if (cursors.writing == insert_cursor)
                    break;
            }
    #else   // Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR
        #error ...
    #endif

        // Now increment the "writing" cursor and commit...
        cur_cursors = m_cursors.load();
        do {
            cursors = Unpack(cur_cursors);
            Y_ASSERT(cursors.writing == insert_cursor);
            cursors.writing += 1;
            new_cursors = Pack(cursors);
        } while (!std::atomic_compare_exchange_strong(&m_cursors, &cur_cursors, new_cursors));

        return true;
    }

    Optional<T> get () const noexcept {
        Optional<T> ret;
        u16 extract_cursor;
        u64 new_cursors;
        u64 cur_cursors;
        Cursors cursors;

        // Reserve one "reading" slot, if available...
        cur_cursors = m_cursors.load();
        do {
            cursors = Unpack(cur_cursors);
            // Inner size must be larger than 0
            if (u16(cursors.writing - cursors.readable) <= 0)
                return ret;
            extract_cursor = cursors.readable;
            cursors.readable += 1;
            new_cursors = Pack(cursors);
        } while (!std::atomic_compare_exchange_strong(&m_cursors, &cur_cursors, new_cursors));

        // Do the actual reading...
        ret = std::move(m_data[extract_cursor % m_capacity]);
        (m_data + (extract_cursor % m_capacity))->T::~T();

        // Spin till the reads that started before this one have finished...
    #if Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 0
        do {
            cursors = Unpack(m_cursors.load());
        } while (cursors.reading != extract_cursor);
    #elif Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 1
        for (;;) {
            cursors = Unpack(m_cursors.load());
            if (cursors.reading == extract_cursor)
                break;
            std::this_thread::yield();
        }
    #elif Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR == 2
        for (int i = 0; i < 100; ++i) {
            cursors = Unpack(m_cursors.load());
            if (cursors.reading == extract_cursor)
                break;
        }
        if (cursors.reading != extract_cursor)
            for (;;) {
                std::this_thread::yield();
                cursors = Unpack(m_cursors.load());
                if (cursors.reading == extract_cursor)
                    break;
            }
    #else   // Y_OPT_LOCKFREE_QUEUE_SPIN_BEHAVIOR
        #error ...
    #endif

        // Now increment the "reading" cursor and commit...
        cur_cursors = m_cursors.load();
        do {
            cursors = Unpack(cur_cursors);
            Y_ASSERT(cursors.reading == extract_cursor);
            cursors.reading += 1;
            new_cursors = Pack(cursors);
        } while (!std::atomic_compare_exchange_strong(&m_cursors, &cur_cursors, new_cursors));

        return ret;
    }

private:
    struct Cursors {
        u16 reading;
        u16 readable;
        u16 writing;
        u16 writable;
    };

    static Cursors Unpack (uint64_t packed_cursor) noexcept {
        Cursors ret;
        ret.reading  = (packed_cursor >>  0) & 0xFFFF;
        ret.readable = (packed_cursor >> 16) & 0xFFFF;
        ret.writing  = (packed_cursor >> 32) & 0xFFFF;
        ret.writable = (packed_cursor >> 48) & 0xFFFF;
        return ret;
    }

    static uint64_t Pack (Cursors cursors) noexcept {
        return
            (u64(cursors.reading ) <<  0) |
            (u64(cursors.readable) << 16) |
            (u64(cursors.writing ) << 32) |
            (u64(cursors.writable) << 48);
    }

private:
    T * const m_data;
    unsigned const m_capacity;
    mutable std::atomic<u64> m_cursors;
};

//======================================================================

    }   // namespace Lockfree
}   // namespace y
