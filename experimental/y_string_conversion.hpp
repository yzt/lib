#pragma once

//======================================================================

#include <type_traits>

//======================================================================

namespace y {
    namespace cvt {

//======================================================================

static constexpr char DigitSet [2][36 + 1] = {
    "0123456789abcdefghijklmnopqrstuvwxyz",
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
};

//======================================================================

struct ToStrResult {
    unsigned size;
    bool succeeded;
};

//======================================================================

template <typename Char, typename UnsignedT>
std::enable_if_t<
    std::is_same_v<UnsignedT, unsigned char> ||
    std::is_same_v<UnsignedT, unsigned short> ||
    std::is_same_v<UnsignedT, unsigned int> ||
    std::is_same_v<UnsignedT, unsigned long> ||
    std::is_same_v<UnsignedT, unsigned long long>,
ToStrResult>
i2s (
    Char * buffer, unsigned size, UnsignedT v,
    unsigned char base = 10, bool uppercase = false,
    unsigned digit_group_size = 0, Char digit_group_sep = '\'',
    bool nul_terminate = false
) {
    static_assert(std::is_integral_v<UnsignedT> && std::is_unsigned_v<UnsignedT>, "Only works for unsigned integer types.");
    ToStrResult ret = {};
    if (buffer && size && 2 <= base && base <= 36) {
        ret.succeeded = true;
        char const * digits = DigitSet[uppercase ? 1 : 0];
        if (0 == digit_group_size)
            digit_group_size = size;
        unsigned grp = 0;
        do {
            Char d = digits[v % base];
            v /= base;
            if (grp == digit_group_size) {
                grp = 0;
                if (ret.size == size) {ret.succeeded = false; break;}
                buffer[ret.size++] = digit_group_sep;
            }
            grp += 1;
            if (ret.size == size) {ret.succeeded = false; break;}
            buffer[ret.size++] = d;
        } while (0 != v);

        if (ret.size > 0)
            for (unsigned i = 0, j = ret.size - 1; i < j; ++i, --j) {
                auto t = buffer[i];
                buffer[i] = buffer[j];
                buffer[j] = t;
            }
        if (nul_terminate)
            if (ret.size >= size) {
                ret.succeeded = false;
            } else {
                buffer[ret.size] = '\0';
            }
    }
    return ret;
}

//----------------------------------------------------------------------

template <typename Char, typename SignedT>
std::enable_if_t<
    std::is_same_v<SignedT, signed char> ||
    std::is_same_v<SignedT, signed short> ||
    std::is_same_v<SignedT, signed int> ||
    std::is_same_v<SignedT, signed long> ||
    std::is_same_v<SignedT, signed long long>,
ToStrResult>
i2s (
    Char * buffer, unsigned size, SignedT v,
    unsigned char base = 10, bool uppercase = false,
    unsigned digit_group_size = 0, Char digit_group_sep = '\'',
    bool nul_terminate = false
) {
    static_assert(std::is_integral_v<SignedT> && std::is_signed_v<SignedT>, "Only works for signed integer types.");
    ToStrResult ret = {};
    if (buffer && size) {
        std::make_unsigned_t<SignedT> x;
        unsigned added_size = 0;
        if (v < 0) {
            x = std::make_unsigned_t<SignedT>(-v);
            *buffer = '-';
            added_size = 1;
        } else
            x = std::make_unsigned_t<SignedT>(v);

        ret = i2s(buffer + added_size, size - added_size, x, base, uppercase, digit_group_size, digit_group_sep, nul_terminate);

        ret.size += added_size;
    }
    return ret;
}

//----------------------------------------------------------------------

template <typename Char, typename RealT>
std::enable_if_t<
    std::is_same_v<RealT, float> ||
    std::is_same_v<RealT, double> ||
    std::is_same_v<RealT, long double>,
ToStrResult>
f2s (
    Char * buffer, unsigned size, RealT v,
    unsigned digit_group_size = 0, Char digit_group_sep = '\'',
    bool uppercase = false,
    bool nul_terminate = false
) {
    frexp
}

//======================================================================

    }   // namespace cvt
}   // namespace y
