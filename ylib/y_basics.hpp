#pragma once

//======================================================================

#define Y_OPT_STD_ASSERT
#define Y_OPT_STD_EXCEPTION
#define Y_OPT_STD_TYPETRAITS

//======================================================================

#if defined(Y_OPT_STD_ASSERT)
    #include <cassert>      // TODO(yzt): remove this when I have a better implementation myself...
#endif

#if defined(Y_OPT_STD_EXCEPTION)
    #include <exception>
#endif

#if defined(Y_OPT_STD_TYPETRAITS)
    #include <type_traits>
#endif

//======================================================================
// Compilers, platforms, and features:
//----------------------------------------------------------------------

#if defined(_MSC_VER)
    #define Y_FEATURE_COMPILER_MSVC         1
    #if _MSC_VER <= 1900    // Visual Studio 2015 and below
        #define Y_FEATURE_MSVC_IS_OLD       1
    #endif
#elif defined(__clang__)
    #define Y_FEATURE_COMPILER_CLANG        1
#elif defined(__GNUC__)
    #define Y_FEATURE_COMPILER_GCC          1
#else
    #error "[Y] Unknown compiler; add something for it here."
#endif

//----------------------------------------------------------------------

#if defined(_WIN32)
    #define Y_FEATURE_OS_WIN                1
#elif __APPLE__   // NOTE(yzt): no defined() needed here.
    #define Y_FEATURE_OS_POSIX              1
    #if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
        #define Y_FEATURE_OS_IOS_SIMULATOR  1
    #elif TARGET_OS_IPHONE
        #define Y_FEATURE_OS_IOS            1
    #elif TARGET_OS_MAC
        #define Y_FEATURE_OS_OSX            1
    #else
        #error "[Y] Unknown Apple product; add something for it here."
    #endif
#elif __linux__
    #define Y_FEATURE_OS_POSIX              1
    #define Y_FEATURE_OS_LINUX              1
#elif __unix__
    #define Y_FEATURE_OS_POSIX              1
    #define Y_FEATURE_OS_UNIX               1
#elif defined(_POSIX_VERSION)
    #define Y_FEATURE_OS_POSIX              1
#else
    #error "[Y] Unknown OS; add something for it here."
#endif

//----------------------------------------------------------------------

// x86; 32- or 64-bit
#if defined(__x86_64__) || defined(_M_X64)
    #define Y_FEATURE_ARCH_64               1
#elif defined(__i386) || defined(_M_IX86)
    #define Y_FEATURE_ARCH_32               1
#endif

// ARM; 32- or 64-bit
#if defined(__aarch64__) || defined(_M_ARM64)
    #define Y_FEATURE_ARCH_64               1
#elif defined(__arm__) || defined(_M_ARM)
    #define Y_FEATURE_ARCH_32               1
#endif

// POWER; 32- or 64-bit
#if defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
    #if defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || defined(__64BIT__) || defined(_LP64) || defined(__LP64__)
        #define Y_FEATURE_ARCH_64           1
    #else
        #define Y_FEATURE_ARCH_32           1
    #endif
#endif

// Itanium; only 64-bit
#if defined(__ia64) || defined(__itanium__) || defined(_M_IA64)
    #define Y_FEATURE_ARCH_64               1
#endif

#if !defined(Y_FEATURE_ARCH_64) && !defined(Y_FEATURE_ARCH_32)
    #error "[Y] Unknown architecture (bits); add something for it here."
#endif

#if defined(Y_FEATURE_ARCH_64) && defined(Y_FEATURE_ARCH_32)
    #error "[Y] Unknown architecture (bits); both 32- and 64-bit macros got defined; something is messed up."
#endif

//----------------------------------------------------------------------
//
//#define Y_FEATURE_ENDIANNESS_BIG        (*(uint16_t const *)"\0\xFF" < 0x100)
//#define Y_FEATURE_ENDIANNESS_LITTLE     (!Y_FEATURE_ENDIANNESS_BIG)

//======================================================================

//#define Y_GIMMICKED_CONSTEXPR         constexpr
//#if defined(Y_FEATURE_COMPILER_MSVC)
//    #pragma warning (disable: 4814)	// in C++14 'constexpr' will not imply 'const'; consider explicitly specifying 'const'
//
//    #undef  Y_GIMMICKED_CONSTEXPR
//    #define Y_GIMMICKED_CONSTEXPR     /**/
//#endif

//======================================================================
// Macros:
//----------------------------------------------------------------------

#define _Y_STRINGIZE_DO(x)          #x
#define Y_STRINGIZE(x)              _Y_STRINGIZE_DO(x)

#define _Y_CONCAT_DO(x, y)          x ## y
#define Y_CONCAT(x, y)              _Y_CONCAT_DO(x, y)

//----------------------------------------------------------------------

#define Y_PTR_VALID(p)              ((p) != nullptr)
#define Y_PTR_DIFF_BYTES(p0, p1)    (reinterpret_cast<char const *>(&*(p1)) - reinterpret_cast<char const *>(&*(p0)))

//----------------------------------------------------------------------

#define Y_THROW_SPEC(...)           /**/

//----------------------------------------------------------------------

#define Y_PLACEMENT_NEW(p)          new (p)

//----------------------------------------------------------------------

#if defined(Y_FEATURE_COMPILER_MSVC)
    #define Y_FORCE_INLINE          __forceinline
    #define Y_UNREACHABLE()         __assume(0)
    #define Y_RESTRICT              __restrict
#else
    #define Y_FORCE_INLINE          inline __attribute__((always_inline))
    #define Y_UNREACHABLE()         __builtin_unreachable()
    #define Y_RESTRICT              __restrict__
#endif

#define Y_DEFAULT_CASE_UNREACHABLE()    default: Y_UNREACHABLE(); break

//======================================================================
// Assertions:
//----------------------------------------------------------------------

#define Y_STATIC_ASSERT(cond, msg)          static_assert(cond, msg)
#define Y_STATIC_ASSERT_SIZE(T, sz)         Y_STATIC_ASSERT(sizeof(T) == sz, "[Y] Size of " Y_STRINGIZE(T) " is expected to be " Y_STRINGIZE(sz) ".")

#if defined(Y_OPT_STD_TYPETRAITS)
    #define Y_STATIC_ASSERT_POD(T)          Y_STATIC_ASSERT(std::is_pod_v<T>, "[Y] Type " Y_STRINGIZE(T) " should be a POD.")
    #define Y_STATIC_ASSERT_TRIVIAL_COPY(T) Y_STATIC_ASSERT(std::is_trivially_copyable_v<T>, "[Y] Type " Y_STRINGIZE(T) " should be trivially copyable.")
#endif

//----------------------------------------------------------------------

#if defined(Y_OPT_STD_ASSERT)
    #define Y_ASSERT(cond, ...)             assert(cond)
    #define Y_ASSERT_STRONG(cond, ...)      assert(cond)
    #define Y_ASSERT_REPAIR(cond, ...)      (cond)
#endif

//======================================================================
// Simple Logs:
//----------------------------------------------------------------------

#if !defined(Y_LOG_FUNC)
    #define Y_LOG_FUNC                      ::printf
#endif

#if Y_FEATURE_COMPILER_MSVC
    #define Y_SIMPLE_LOG(cat, fmt, ...)     Y_LOG_FUNC("[" cat "] " fmt "\n", __VA_ARGS__)
    #define Y_SIMPLE_TRACE(fmt, ...)        Y_SIMPLE_LOG("TRACE", fmt, __VA_ARGS__)
    #define Y_SIMPLE_INFO(fmt, ...)         Y_SIMPLE_LOG("INFO", fmt, __VA_ARGS__)
    #define Y_SIMPLE_WARN(fmt, ...)         Y_SIMPLE_LOG("WARN", fmt, __VA_ARGS__)
    #define Y_SIMPLE_ERROR(fmt, ...)        Y_SIMPLE_LOG("ERROR", fmt, __VA_ARGS__)
    #define Y_SIMPLE_FATAL(fmt, ...)        Y_SIMPLE_LOG("FATAL", fmt, __VA_ARGS__)
#else
    #define Y_SIMPLE_LOG(cat, fmt, ...)     Y_LOG_FUNC("[" cat "] " fmt "\n", ## __VA_ARGS__)
    #define Y_SIMPLE_TRACE(fmt, ...)        Y_SIMPLE_LOG("TRACE", fmt, __VA_ARGS__)
    #define Y_SIMPLE_INFO(fmt, ...)         Y_SIMPLE_LOG("INFO", fmt, __VA_ARGS__)
    #define Y_SIMPLE_WARN(fmt, ...)         Y_SIMPLE_LOG("WARN", fmt, __VA_ARGS__)
    #define Y_SIMPLE_ERROR(fmt, ...)        Y_SIMPLE_LOG("ERROR", fmt, __VA_ARGS__)
    #define Y_SIMPLE_FATAL(fmt, ...)        Y_SIMPLE_LOG("FATAL", fmt, __VA_ARGS__)
#endif

//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//======================================================================

namespace y {

//======================================================================

using I8  = signed char;
using U8  = unsigned char;
using I16 = signed short;
using U16 = unsigned short;
using I32 = signed int;
using U32 = unsigned int;
using I64 = signed long long;
using U64 = unsigned long long;

using F32 = float;
using F64 = double;

using Byte = U8;

using CStr = char *;
using CCStr = char const *;

//----------------------------------------------------------------------

#if defined(Y_FEATURE_ARCH_64)
    using Size = U64;
    using PtrDiff = I64;
    using IntPtr = I64;
#elif defined(Y_FEATURE_ARCH_32)
    using Size = U32;
    using PtrDiff = I32;
    using IntPtr = I32;
#endif

//======================================================================

template <typename T> struct RemoveRefType {using Type = T;};
template <typename T> struct RemoveRefType <T &> {using Type = T;};
template <typename T> struct RemoveRefType <T &&> {using Type = T;};
template <typename T> using RemoveRef = typename RemoveRefType<T>::Type;

template <typename T> inline constexpr T && Fwd (RemoveRef<T> & v) noexcept {return static_cast<T &&>(v);}
template <typename T> inline constexpr T && Fwd (RemoveRef<T> && v) noexcept {return static_cast<T &&>(v);}

template <typename T> inline constexpr RemoveRef<T> && Mov (T && v) noexcept {return static_cast<RemoveRef<T> &&>(v);}

//======================================================================
//======================================================================

//template <typename T>
//inline constexpr auto Min (T && a, T && b) {return b < a ? Fwd<T>(b) : Fwd<T>(a);}
//
//template <typename T>
//inline constexpr auto Max (T && a, T && b) {return b < a ? Fwd<T>(a) : Fwd<T>(b);}

//----------------------------------------------------------------------

template <typename T>
inline constexpr auto
Min (T const & a, T const & b) {
    return b < a ? b : a;
}

template <typename T>
inline constexpr auto
Max (T const & a, T const & b) {
    return b < a ? a : b;
}

template <typename T>
inline constexpr auto
Min (T && a, T && b) {
    return b < a ? Fwd<T>(b) : Fwd<T>(a);
}

template <typename T>
inline constexpr auto
Max (T && a, T && b) {
    return b < a ? Fwd<T>(a) : Fwd<T>(b);
}

//----------------------------------------------------------------------

template <typename T>
inline constexpr auto
MinOf (T && v) {
    return Fwd<T>(v);
}

template <typename T, typename... Ts>
inline constexpr auto
MinOf (T && v0, T && v1, Ts && ... vs) {
    return v1 < v0
        ? MinOf(Fwd<T>(v1), Fwd<Ts>(vs)...)
        : MinOf(Fwd<T>(v0), Fwd<Ts>(vs)...);
}

template <typename T>
inline constexpr auto
MaxOf (T && v) {
    return Fwd<T>(v);
}

template <typename T, typename... Ts>
inline constexpr auto
MaxOf (T && v0, T && v1, Ts && ... vs) {
    return v1 < v0
        ? MinOf(Fwd<T>(v0), Fwd<Ts>(vs)...)
        : MinOf(Fwd<T>(v1), Fwd<Ts>(vs)...);
}

//----------------------------------------------------------------------

template <typename T>
inline constexpr bool In (T &&) {
    return false;
}

template <typename T, typename T0, typename... Ts>
inline constexpr bool
In (T && v, T0 && v0, Ts && ... vs) {
    return Fwd<T>(v) == Fwd<T0>(v0)
        ? true
        : In(Fwd<T>(v), Fwd<Ts>(vs)...);
}

//----------------------------------------------------------------------

template <typename T>
inline constexpr void
Swap (T & a, T & b) noexcept {    // NOTE(yzt): noexcept is not correct if T's move is not noexcept
    auto temp = Mov(a);
    a = Mov(b);
    b = Mov(temp);
}

//======================================================================

template <typename T, typename U>
T const *
BinarySearch (T const * begin, T const * end, U const & val) {
    auto lo = begin, hi = end;
    while (lo < hi) {
        auto mid = lo + (hi - lo) / 2;
        if (val < *mid) hi = mid;
        else if (*mid < val) lo = mid + 1;
        else return mid;
    }
    return hi;
}

//----------------------------------------------------------------------

template <typename T, typename U>
T *
BinarySearch (T * begin, T * end, U const & val) {
    auto lo = begin, hi = end;
    while (lo < hi) {
        auto mid = lo + (hi - lo) / 2;
        if (val < *mid) hi = mid;
        else if (*mid < val) lo = mid + 1;
        else return mid;
    }
    return hi;
}

//----------------------------------------------------------------------
//======================================================================

class Exception : public std::exception {
public:
    explicit Exception (char const * msg) noexcept : m_msg (msg) {}
    virtual char const * what () const noexcept override {return m_msg;}

private:
    char const * m_msg;
};

//----------------------------------------------------------------------

struct OutOfMemException : Exception {
    explicit OutOfMemException (char const * msg) noexcept : Exception(msg) {}
};

//----------------------------------------------------------------------

struct BadAccessException : Exception {
    BadAccessException (char const * msg) : Exception (msg) {}
};

//----------------------------------------------------------------------
//======================================================================

struct Blob {
    Byte * ptr;
    Byte * end;

    bool null () const {return !Y_PTR_VALID(ptr) || !Y_PTR_VALID(end) || end < ptr;}
    bool empty () const {return end <= ptr;}
    auto size () const {return end - ptr;}
};

struct Buffer : Blob {
    //Byte * ptr;
    //Byte * end;
    Byte * cap;

    bool null () const {return !Y_PTR_VALID(ptr) || !Y_PTR_VALID(end) || !Y_PTR_VALID(cap) || end < ptr || cap < end;}  // Note(yzt): Don't Panic! It's OK.
    //bool empty () const {return ptr == end;}
    //auto size () const {return end - ptr;}
    bool full () const {return cap <= end;}
    auto capacity () const {return cap - ptr;}
    auto remaining () const {return cap - end;}
};

//----------------------------------------------------------------------

template <typename T>
struct BufferT {
    T * ptr;
    T * end;
    T * cap;

    BufferT () = default;
    BufferT (Buffer const & b)
        : ptr (static_cast<T *>(b.ptr))
        , end (static_cast<T *>(b.end))
        , cap (static_cast<T *>(b.cap))
    {
        Y_ASSERT((b.size() % sizeof(T)) == 0);
        Y_ASSERT((b.capacity() % sizeof(T)) == 0);
    }

    operator Buffer () const {
        return {
            static_cast<Byte *>(ptr),
            static_cast<Byte *>(end),
            static_cast<Byte *>(cap),
        };
    }

    bool null () const {return !Y_PTR_VALID(ptr) || !Y_PTR_VALID(end) || !Y_PTR_VALID(cap) || end < ptr || cap < end;}
    bool empty () const {return end <= ptr;}
    auto size () const {return end - ptr;}
    bool full () const {return cap <= end;}
    auto capacity () const {return cap - ptr;}
    auto remaining () const {return cap - end;}
};

//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------

struct MemReader {
    Byte const * ptr;
    Byte const * end;

    MemReader (Byte const * ptr_, Byte const * end_) noexcept : ptr (ptr_), end (end_) {Y_ASSERT(ptr <= end);}
    MemReader (Byte const * ptr_, Size size) noexcept : ptr (ptr_), end (ptr_ + size) {}

    bool null () const {return !Y_PTR_VALID(ptr) || !Y_PTR_VALID(end) || end < ptr;}
    auto size () const noexcept {return Y_PTR_DIFF_BYTES(ptr, end);}
    bool empty () const noexcept {return end <= ptr;}

    MemReader tell () const noexcept {return *this;}
    void seek (MemReader const & that) noexcept {*this = that;}

    template <typename T>
    bool can_read () const noexcept {
        Y_STATIC_ASSERT_POD(T);
        return sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end);
    }

    template <typename T>
    T read () {
        Y_STATIC_ASSERT_POD(T);
        if (Y_PTR_DIFF_BYTES(ptr, end) < sizeof(T))
            throw BadAccessException {"Cursor has no room."};
        auto ret = reinterpret_cast<T const *>(ptr);
        ptr += sizeof(T);
        return *ret;
    }

    template <typename T>
    bool read (T & out_value) noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        bool ret = false;
        if (sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end)) {
            out_value = *reinterpret_cast<T const *>(ptr);
            ptr += sizeof(T);
            ret = true;
        }
        return ret;
    }

    template <typename T>
    bool skip () noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        bool ret = false;
        auto rem = Y_PTR_DIFF_BYTES(ptr, end);
        if (sizeof(T) <= rem) {
            ptr += sizeof(T);
            ret = true;
        } else {
            ptr += rem;
        }
        return ret;
    }

    bool skip (size_t bytes) noexcept {
        bool ret = false;
        auto rem = Y_PTR_DIFF_BYTES(ptr, end);
        if (PtrDiff(bytes) <= rem) {
            ptr += bytes;
            ret = true;
        } else {
            ptr += rem;
        }
        return ret;
    }
};

//----------------------------------------------------------------------

struct MemWriter {
    Byte * ptr;
    Byte * end;

    MemWriter (Byte * ptr_, Byte * end_) noexcept : ptr (ptr_), end (end_) {Y_ASSERT(ptr <= end);}
    MemWriter (Byte * ptr_, Size size) noexcept : ptr (ptr_), end (ptr_ + size) {}

    bool null () const noexcept {return !Y_PTR_VALID(ptr) || !Y_PTR_VALID(end) || end < ptr;}
    auto size () const noexcept {return Y_PTR_DIFF_BYTES(ptr, end);}
    bool empty () const noexcept {return end <= ptr;}

    MemWriter tell () const noexcept {return *this;}
    void seek (MemWriter const & that) noexcept {*this = that;}

    template <typename T>
    bool can_read () const noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        return sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end);
    }

    template <typename T>
    T read () {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        if (Y_PTR_DIFF_BYTES(ptr, end) < sizeof(T))
            throw BadAccessException {"Cursor has no room."};
        auto ret = reinterpret_cast<T const *>(ptr);
        ptr += sizeof(T);
        return *ret;
    }

    template <typename T>
    bool read (T & out_value) noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        bool ret = false;
        if (sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end)) {
            out_value = *reinterpret_cast<T const *>(ptr);
            ptr += sizeof(T);
            ret = true;
        }
        return ret;
    }

    template <typename T>
    bool can_write () const noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        return sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end);
    }

    template <typename T>
    bool write (T const & in) noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        bool ret = false;
        if (sizeof(T) <= Y_PTR_DIFF_BYTES(ptr, end)) {
            *reinterpret_cast<T *>(ptr) = in;
            ptr += sizeof(T);
            ret = true;
        }
        return ret;
    }

    template <typename T>
    bool skip () noexcept {
        Y_STATIC_ASSERT_TRIVIAL_COPY(T);
        bool ret = false;
        auto rem = Y_PTR_DIFF_BYTES(ptr, end);
        if (sizeof(T) <= rem) {
            ptr += sizeof(T);
            ret = true;
        } else {
            ptr += rem;
        }
        return ret;
    }

    bool skip (size_t bytes) noexcept {
        bool ret = false;
        auto rem = Y_PTR_DIFF_BYTES(ptr, end);
        if (ptrdiff_t(bytes) <= rem) {
            ptr += bytes;
            ret = true;
        } else {
            ptr += rem;
        }
        return ret;
    }
};

//======================================================================

template <typename CharType>
Size StrLen (CharType const * s) {
    Size ret = 0;
    if (Y_PTR_VALID(s))
        while (*s++)
            ret++;
    return ret;
}

//======================================================================

}   // namespace y

//======================================================================

Y_STATIC_ASSERT_SIZE(y::I8, 1);
Y_STATIC_ASSERT_SIZE(y::U8, 1);
Y_STATIC_ASSERT_SIZE(y::I16, 2);
Y_STATIC_ASSERT_SIZE(y::U16, 2);
Y_STATIC_ASSERT_SIZE(y::I32, 4);
Y_STATIC_ASSERT_SIZE(y::U32, 4);
Y_STATIC_ASSERT_SIZE(y::I64, 8);
Y_STATIC_ASSERT_SIZE(y::U64, 8);
Y_STATIC_ASSERT_SIZE(y::F32, 4);
Y_STATIC_ASSERT_SIZE(y::F64, 8);
Y_STATIC_ASSERT_SIZE(y::Byte, 1);
Y_STATIC_ASSERT_SIZE(y::Size, sizeof(void *));
Y_STATIC_ASSERT_SIZE(y::PtrDiff, sizeof(void *));
Y_STATIC_ASSERT_SIZE(y::IntPtr, sizeof(void *));

#if defined(Y_FEATURE_ARCH_32)
    Y_STATIC_ASSERT_SIZE(void *, 4);
#elif defined(Y_FEATURE_ARCH_64)
    Y_STATIC_ASSERT_SIZE(void *, 8);
#endif

//======================================================================
