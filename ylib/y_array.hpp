#pragma once 

#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace y {

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

}   // namespace y
