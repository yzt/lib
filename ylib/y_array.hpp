#pragma once 

#include <cstddef>
#include <cstdint>
#include <initializer_list>

//======================================================================

namespace y {

//======================================================================

#if 0
namespace details {
    constexpr uint8_t FitsInHowManyBytes (size_t n) {
        return (n <= 0xFF) ? 1
            : ((n <= 0xFFFF) ? 2
            : ((n <= 0xFFFF'FFFF) ? 4
            : 8));
    }

    template <uint8_t Bytes>
    struct SizedUIntType;

    template <> struct SizedUIntType<1> {using Type = uint8_t; };
    template <> struct SizedUIntType<2> {using Type = uint16_t;};
    template <> struct SizedUIntType<4> {using Type = uint32_t;};
    template <> struct SizedUIntType<8> {using Type = uint64_t;};
}   // namespace details

template <typename T>
class RevPointer {
public:
    constexpr RevPointer () : m_ptr (nullptr) {}
    constexpr RevPointer (T * p) : m_ptr (p) {}
    constexpr RevPointer (RevPointer<T> const & that) : m_ptr (that.m_ptr) {}
    constexpr RevPointer (RevPointer<T> && that) : m_ptr (that.m_ptr) {/*that.m_ptr = nullptr;*/}

    constexpr RevPointer<T> & operator = (ReversePointer<T> const & that) {m_ptr = that.m_ptr; return *this;}
    constexpr RevPointer<T> & operator = (ReversePointer<T> && that) {m_ptr = that.m_ptr; return *this;}

    explicit operator bool () const {return m_ptr != nullptr;}

    T & operator * () {return *m_ptr;}
    T const & operator * () const {return *m_ptr;}
    T * operator -> () {return m_ptr;}
    T const * operator -> () const {return m_ptr;}

    bool operator == (RevPointer<T> const & that) const {return m_ptr == that.m_ptr;}
    bool operator != (RevPointer<T> const & that) const {return m_ptr != that.m_ptr;}
    bool operator <  (RevPointer<T> const & that) const {return m_ptr >= that.m_ptr;}
    bool operator <= (RevPointer<T> const & that) const {return m_ptr >  that.m_ptr;}
    bool operator >  (RevPointer<T> const & that) const {return m_ptr <= that.m_ptr;}
    bool operator >= (RevPointer<T> const & that) const {return m_ptr <  that.m_ptr;}

    RevPointer<T> & operator ++ () {m_ptr -= 1; return *this;}
    RevPointer<T> & operator -- () {m_ptr += 1; return *this;}

    RevPointer<T> & operator += (int diff) {m_ptr -= diff; return *this;}
    RevPointer<T> & operator -= (int diff) {m_ptr += diff; return *this;}

    RevPointer<T> operator + (int diff) {return {m_ptr - diff};}
    RevPointer<T> operator - (int diff) {return {m_ptr + diff};}

private:
    T * m_ptr = nullptr;
};

template <typename T>
class RevConstPointer {
public:
    constexpr RevConstPointer () : m_ptr (nullptr) {}
    constexpr RevConstPointer (T * p) : m_ptr (p) {}
    constexpr RevConstPointer (RevConstPointer<T> const & that) : m_ptr (that.m_ptr) {}
    constexpr RevConstPointer (RevConstPointer<T> && that) : m_ptr (that.m_ptr) {/*that.m_ptr = nullptr;*/}

    constexpr RevConstPointer<T> & operator = (RevConstPointer<T> const & that) {m_ptr = that.m_ptr; return *this;}
    constexpr RevConstPointer<T> & operator = (RevConstPointer<T> && that) {m_ptr = that.m_ptr; return *this;}

    explicit operator bool () const {return m_ptr != nullptr;}

    T const & operator * () const {return *m_ptr;}
    T const * operator -> () const {return m_ptr;}

    bool operator == (RevConstPointer<T> const & that) const {return m_ptr == that.m_ptr;}
    bool operator != (RevConstPointer<T> const & that) const {return m_ptr != that.m_ptr;}
    bool operator <  (RevConstPointer<T> const & that) const {return m_ptr >= that.m_ptr;}
    bool operator <= (RevConstPointer<T> const & that) const {return m_ptr >  that.m_ptr;}
    bool operator >  (RevConstPointer<T> const & that) const {return m_ptr <= that.m_ptr;}
    bool operator >= (RevConstPointer<T> const & that) const {return m_ptr <  that.m_ptr;}

    RevConstPointer<T> & operator ++ () {m_ptr -= 1; return *this;}
    RevConstPointer<T> & operator -- () {m_ptr += 1; return *this;}

    RevConstPointer<T> & operator += (int diff) {m_ptr -= diff; return *this;}
    RevConstPointer<T> & operator -= (int diff) {m_ptr += diff; return *this;}

    RevConstPointer<T> operator + (int diff) {return {m_ptr - diff};}
    RevConstPointer<T> operator - (int diff) {return {m_ptr + diff};}

private:
    T * m_ptr = nullptr;
};

template <typename T, size_t Size_>
class Array {
public:
    using ValueType = T;
    using SizeType = size_t;
    using MyType = Array<T, Capacity_>;
    using IterType = T *;
    using IterConstType = T const *;
    using RevIterType = RevPointer<T>;
    using RevIterConstType = RevConstPointer<T>;
    static constexpr bool IsFixedSize = true;
    static constexpr bool IsFixedCapacity = true;
    static constexpr SizeType Size = Size_;
    static constexpr SizeType Capacity = Size_;

public:
    constexpr Array () = default;
    template <typename U>
    constexpr Array (U const & fill_value) {
        for (auto & e : m_data)
            e = fill_value;
    }
    template <typename U>
    constexpr Array (std::initializer_list<U> init) {...}
    template <typename U, size_t OtherSize>
    constexpr Array (Array<U, OtherSize> const & that) {...}
    constexpr Array (MyType && that) {...}
    ~Array () = default;

    constexpr SizeType size () const {return Size;}
    constexpr SizeType capacity () const {return Size;}
    constexpr T * data () {return m_data;}
    constexpr T const * data () const {return m_data;}

    constexpr IterType begin () {return m_data;}
    constexpr IterType end () {return m_data + Size;}
    constexpr IterConstType begin () const {return m_data;}
    constexpr IterConstType end () const {return m_data + Size;}
    constexpr IterConstType cbegin () const {return m_data;}
    constexpr IterConstType cend () const {return m_data + Size;}

    constexpr RevIterType rbegin () {return m_data + Size - 1;}
    constexpr RevIterType rend () {return m_data - 1;}
    constexpr RevIterConstType rbegin () const {return m_data + Size - 1;}
    constexpr RevIterConstType rend () const {return m_data - 1;}
    constexpr RevIterConstType crbegin () const {return m_data + Size - 1;}
    constexpr RevIterConstType crend () const {return m_data - 1;}

    constexpr ValueType & operator [] (SizeType index) {return m_data[index];}
    constexpr ValueType const & operator [] (SizeType index) const {return m_data[index];}

private:
    T m_data [Size];
};

template <typename T, size_t Capacity_>
class FixedVector {
public:
    using ValueType = T;
    using SizeType = typename details::SizedUIntType<details::FitsInHowManyBytes(Capacity_)>::Type;
    using MyType = FixedVector<T, Capacity_>;
    static constexpr bool IsFixedSize = false;
    static constexpr bool IsFixedCapacity = true;
    static constexpr SizeType Capacity = Capacity_;

public:
    
private:
    SizeType m_size;
    T m_data [Capacity];
};

template <typename T>
class Vector {
public:
    using ValueType = T;
    using SizeType = size_t;
    using MyType = Vector<T>;
    static constexpr bool IsFixedSize = false;
    static constexpr bool IsFixedCapacity = false;

public:

private:
    T * m_data;
    SizeType m_size;
    SizeType m_capacity;
};
#endif

//======================================================================

/*
Basically, there are a few kinds of buffers/arrays/blobs, depending on how
they allocate/deal with their memory/elements:
    1. CF:    constant capacity,    fixed length (a C array)
    2. CV:    constant capacity, variable length (FixedArray or CappedArray)
    3. IF: initialized capacity,    fixed length (dyn_array?)
    4. IV: initialized capacity, variable length (non-resizing std::vector?)
    5. DF:     dynamic capacity,    fixed length (realloc()ing a buffer?)
    6. DV:     dynamic capacity, variable length (std::vector)

Constant capacity means compile-time, and templated capacity.
Initialized capacity means it is set at construction time and no reallocs.
Dynamic capacity means the underlying buffer is allocated and reallocated
at runtime.

Fixed length means it's the same as capacity and all elements are always constructed and available.
Variable length means you can insert/erase elements.

(Maybe fixed-length variations should be called array and
variable-length ones should be called vector.)

Glossary:
    * "cap" or "capacity" is the maximum number of elements the current
        buffer (whatever and wherever that is) can hold.
    * "len" or "length" is the current number of useable elements (always
        less than or equal to capacity.)
    * "size" is used for both capacity and length, when these are the same
        thing, i.e. in fixed-length variations or situations.
    * "ptr" is a pointer to the accompanying data, when it is stored
        somewhere else (i.e. away from the len/cap/size and ptr itself.)
    * "data" is the name of the array holding the data, when it is right
        there inside the datastructure or along the metadata.
*/

//======================================================================

using RangeSize = intptr_t; // This is not strictly correct, but I want it to be signed.
constexpr RangeSize InvalidRangeSize = -1;

static_assert(RangeSize(-1) < RangeSize(0), "[ERROR] RangeSize must be a *signed* type.");

//----------------------------------------------------------------------

template <typename T>
struct RangeRd {
    RangeSize size;
    T const * ptr;

    constexpr bool empty () const {return !ptr || size <= 0;}
    constexpr T const & operator [] (RangeSize index) const {return ptr;}
    constexpr T const * begin () const {return ptr;}
    constexpr T const * end () const {return ptr + size;}
};

template <typename T>
struct RangeWr {
    RangeSize size;
    T * ptr;

    constexpr bool empty () const {return !ptr || size <= 0;}
    constexpr T const & operator [] (RangeSize index) const {return ptr;}
    constexpr T const * begin () const {return ptr;}
    constexpr T const * end () const {return ptr + size;}

    operator RangeRd<T> () const {return {size, ptr};}
    T & operator [] (RangeSize index) {return ptr;}
    T * begin () {return ptr;}
    T * end () {return ptr + size;}
};

template <typename T>
struct SliceWr {
    RangeSize cap;
    RangeSize len;
    T * ptr;
};

//----------------------------------------------------------------------

inline RangeSize
RangeClamp (RangeSize index, RangeSize low_inc, RangeSize high_inc) {
    return index < low_inc ? low_inc : (index > high_inc ? high_inc : index);
}

//----------------------------------------------------------------------

template <typename T, RangeSize N>
class Array {
public:
//    using Capacity = N;
    static constexpr RangeSize Size = N;
//    using Length = N;
    using MyType = Array<T, N>;
    using ValueType = T;

public:
    constexpr Array () = default;
    constexpr Array (MyType const & that) = default;
    constexpr Array (MyType && that) = default;
    constexpr Array & operator = (MyType const & that) = default;
    constexpr Array & operator = (MyType && that) = default;
    //~Array () = default;

    constexpr RangeSize size () const {return Size;}
    constexpr ValueType const * data () const {return m_data;}
    ValueType * data () {return m_data;}

    constexpr ValueType const & operator [] (RangeSize index) const noexcept {return m_data[index];}
    ValueType & operator [] (RangeSize index) noexcept {return m_data[index];}

    constexpr ValueType const * cbegin () const noexcept {return m_data;}
    constexpr ValueType const * cend () const noexcept {return m_data + Size;}
    constexpr ValueType const * begin () const noexcept {return m_data;}
    constexpr ValueType const * end () const noexcept {return m_data + Size;}
    ValueType * begin () noexcept {return m_data;}
    ValueType * end () noexcept {return m_data + Size;}

    constexpr RangeRd<T> all () const noexcept {return {Size, m_data};}
    constexpr RangeRd<T> to (RangeSize excluded_end_index) const noexcept {auto const y = RangeClamp(excluded_end_index, 0, Size); return {y, m_data};}
    constexpr RangeRd<T> from (RangeSize included_start_index) const noexcept {auto const x = RangeClamp(included_start_index, 0, Size); return {Size - x, m_data + x};}
    constexpr RangeRd<T> some (RangeSize included_start_index, RangeSize excluded_end_index) const noexcept { auto const x = RangeClamp(included_start_index, 0, Size); auto const y = RangeClamp(excluded_end_index, x, Size); return {y - x, m_data + x};}
    
    RangeWr<T> all () noexcept {return {Size, m_data};}
    RangeWr<T> to (RangeSize excluded_end_index) noexcept {auto const y = RangeClamp(excluded_end_index, 0, Size); return {y, m_data};}
    RangeWr<T> from (RangeSize included_start_index) noexcept {auto const x = RangeClamp(included_start_index, 0, Size); return {Size - x, m_data + x};}
    RangeWr<T> some (RangeSize included_start_index, RangeSize excluded_end_index) noexcept {auto const x = RangeClamp(included_start_index, 0, Size); auto const y = RangeClamp(excluded_end_index, x, Size); return {y - x, m_data + x};}

private:
    ValueType m_data [Size];
};

template <typename T, RangeSize Capacity>
class FixedVector {
public:
private:
    RangeSize m_len;
    alignas(T) char m_data [sizeof(T) * Capacity];
};

template <typename T>
struct Blob {
    RangeSize cap;
    RangeSize len;
    T data [];
};

//----------------------------------------------------------------------
//======================================================================

template <typename T>
Blob<T> *
AllocateBlob (RangeSize capacity, RangeSize length = 0, T const & init_value = T()) {
    return nullptr;
}

//----------------------------------------------------------------------

template <typename T>
Blob<T> *   // length will be min(old_length, new_capacity)
ReallocateBlob (Blob<T> * old_blob, RangeSize new_capacity) {
    return nullptr;
}

//----------------------------------------------------------------------
//======================================================================

}   // namespace y

//======================================================================
