#pragma once

//======================================================================

#define Y_OPT_FMT_SUPPORT_STDIO                     1
#define Y_OPT_FMT_SUPPORT_IOSTREAM                  1
#define Y_OPT_FMT_SUPPORT_STD_STRING                1
#define Y_OPT_FMT_STD_STRING_OUTPUT_PRECALC_SIZE    1
#define Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE            100
#define Y_OPT_FMT_MAX_INTEGER_NUM_CHAR_SIZE         64

//======================================================================

//#define _HAS_COMPLETE_CHARCONV  1

#include <cstdint>
#include <utility>  // std::forward
#include <charconv> // std::to_chars() for float/double/long double
#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
    #include <string>
    #include <string_view>
#endif
#if defined(Y_OPT_FMT_SUPPORT_STDIO)
    #include <cstdio>
#endif
#if defined(Y_OPT_FMT_SUPPORT_IOSTREAM)
    #include <ostream>
#endif

//======================================================================
/* The format string has the following format:
 *  any character except '{' is passed through.
 *  two successive curlay braces "{{" will emit a curly brace.
 *  something between '{' and '}' is a format spec, with the following structure:
 *
 *      '{' arg_num [':' width] ['.' precision] ['-' fill_char] [',' base] [case] [justify] [sign] '}'
 *      '{' arg_num ['w' width] ['p' precision] ['f' fill_char] ['b' base] [case] [justify] [sign] '}'
 *      "arg_num"is from 0 onwards, the number of the argument that will be put in place of this format spec
 *      "width" is in [0..254], the minimum width of the whole field
 *      "precision" is in [0..254], the number of digits after decimal
 *      "fill_char" is a single character that should be used for padding on right-aligned values. Default is ' '.
 *      "base" is in [2..36]
 *      //"sign" is one of {'+', '_'}, for always displaying the sign, or displaying either '-' or a space. Default is to only emit '-' for negative numbers.
 *      "justify" is exactly one of {'c', 'C', 'r'} for center-left, center-right, and right. Default is left-justified.
 *      //"case" is 'U', which asks for upper-case stuff (hex digits, the 'e' in scientific representation, etc.) Default is lower-case.
 */
//======================================================================

namespace y {
    namespace fmt {

//======================================================================
//======================================================================

        namespace cvt {

//----------------------------------------------------------------------

inline constexpr char HexDigits [17] = "0123456789ABCDEF";

//----------------------------------------------------------------------

enum class Justify : uint8_t {
    Left = 0,
    CenterLeft,
    CenterRight,
    Right,
};

enum class Case : uint8_t {
    Lower = 0,
    Upper,
};

enum class Sign : uint8_t {
    OnlyNeg = 0,    // only '-'
    Always,         // '-' or '+'
    LeaveRoom,      // '-' or ' '
};

struct Flags {
    union {
        struct {
            bool width : 1;
            bool precision : 1;
            bool fill : 1;
            bool base : 1;
            //bool sign : 1;
            bool justify : 1;
            //bool case_ : 1;
        } overridden;
        uint8_t has_any_non_defaults;
    };
    uint8_t index;
    uint8_t width;
    uint8_t precision;
    char fill;
    uint8_t base : 6;
    //Sign sign : 2;
    Justify justify : 2;
    //Case case_ : 1;
    bool explicit_index : 1;
    bool escaped_curly_brace : 1;
    bool valid;
};
static_assert(sizeof(Flags) == 8, "");

//template <typename OutF, typename T>
//void ToStr (OutF && out, T const & v, Flags flags);

//----------------------------------------------------------------------

template <typename OutF>
void ToStr (OutF && out, char const * v) {
    if (v)
        while (*v)
            out(*v++);
    //else
    //    ToStr(std::forward<OutF>(out), "(null)");
}

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string<C> const & v) {
    for (auto c : v)
        out(c);
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string_view<C> const & v) {
    for (auto c : v)
        out(c);
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

template <typename OutF>
void ToStr (OutF && out, unsigned long long v) {
    int cnt = 0;
    char buffer [20];
    do {
        buffer[cnt++] = '0' + (v % 10);
        v /= 10;
    } while (v > 0);
    for (int i = cnt - 1; i >= 0; --i)
        out(buffer[i]);
}

template <typename OutF>
void ToStr (OutF && out, signed long long v) {
    if (v >= 0) {
        return ToStr(std::forward<OutF>(out), (unsigned long long)(v));
    } else {
        out('-');
        return ToStr(std::forward<OutF>(out), (unsigned long long)(-v));
    }
}

template <typename OutF>
void ToStr (OutF && out, unsigned int v) {return ToStr(std::forward<OutF>(out), (unsigned long long)(v));}
template <typename OutF>
void ToStr (OutF && out, signed int v) {return ToStr(std::forward<OutF>(out), (signed long long)(v));}
template <typename OutF>
void ToStr (OutF && out, unsigned short v) {return ToStr(std::forward<OutF>(out), (unsigned long long)(v));}
template <typename OutF>
void ToStr (OutF && out, signed short v) {return ToStr(std::forward<OutF>(out), (signed long long)(v));}
template <typename OutF>
void ToStr (OutF && out, unsigned char v) {return ToStr(std::forward<OutF>(out), (unsigned long long)(v));}
template <typename OutF>
void ToStr (OutF && out, signed char v) {out(v);}
template <typename OutF>
void ToStr (OutF && out, char v) {out(v);}

template <typename OutF>
void ToStr (OutF && out, bool v) {out(v ? '1' : '0');}

template <typename OutF>
void ToStr (OutF && out, float v) {
    char buffer [Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE];
    auto res = std::to_chars(buffer, buffer + sizeof(buffer), v);
    for (char const * p = buffer; p != res.ptr; ++p)
        out(*p);
}

template <typename OutF>
void ToStr (OutF && out, double v) {
    char buffer [Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE];
    auto res = std::to_chars(buffer, buffer + sizeof(buffer), v);
    for (char const * p = buffer; p != res.ptr; ++p)
        out(*p);
}

template <typename OutF>
void ToStr (OutF && out, long double v) {
    char buffer [Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE];
    auto res = std::to_chars(buffer, buffer + sizeof(buffer), v);
    for (char const * p = buffer; p != res.ptr; ++p)
        out(*p);
}

template <typename OutF>
void ToStr (OutF && out, void const * v) {
    constexpr unsigned shift_down = sizeof(v) * 8 - 8;
    auto x = reinterpret_cast<uintptr_t>(v);
    for (int i = 0; i < sizeof(v); ++i) {
        uint8_t b = (x >> (sizeof(v) * 8 - 8)) & 0xFF;
        out(HexDigits[(b >> 4) & 0x0F]);
        out(HexDigits[(b >> 0) & 0x0F]);
        x <<= 8;
    }
}

//----------------------------------------------------------------------

// Handled width, justification, fill, sign
template <typename OutF, typename Char>
void ToStrHelper (OutF && out, Char const * str, unsigned size, Flags const flags/*, bool is_numeric = false, bool is_negative = false*/) {
    unsigned target_width = (flags.overridden.width ? flags.width : size);
    //bool needs_sign = (is_numeric && (is_negative || (flags.overridden.sign && flags.sign != Sign::OnlyNeg)));
    unsigned used_width = size;// + (needs_sign ? 1 : 0);
    if (target_width < used_width) target_width = used_width;
    Char fill_char = (flags.overridden.fill ? flags.fill : ' ');
    unsigned total_pad = unsigned(target_width - used_width);
    unsigned pad_before = 0, pad_after = total_pad;
    if (flags.overridden.justify) {
        if (flags.justify == Justify::Right) {pad_before = total_pad; pad_after = 0;}
        else if (flags.justify == Justify::CenterLeft) {pad_before = total_pad / 2; pad_after = (total_pad + 1) / 2;}
        else if (flags.justify == Justify::CenterRight) {pad_before = (total_pad + 1) / 2; pad_after = total_pad / 2;}
    }

    for (unsigned i = 0; i < pad_before; ++i) out(fill_char);
    //if (needs_sign) out(is_negative ? '-' : (flags.sign == Sign::Always ? '+' : ' '));
    for (unsigned i = 0; i < size; ++i) out(str[i]);
    for (unsigned i = 0; i < pad_after; ++i) out(fill_char);
}

namespace detail {
template <typename Char>
inline unsigned StrLen (Char const * s) {
    unsigned ret = 0;
    if (s)
        while (*s++)
            ret++;
    return ret;
}
}

//======================================================================
// ToStr for built-in types:
//----------------------------------------------------------------------
// Handle string types:
//----------------------------------------------------------------------

template <typename OutF, typename CharT>
std::enable_if_t<
    std::is_same_v<CharT, char> ||
    std::is_same_v<CharT, wchar_t> ||
    std::is_same_v<CharT, char16_t> ||
    std::is_same_v<CharT, char32_t>>
ToStr (OutF && out, CharT const * v, Flags flags) {
    if (0 == flags.has_any_non_defaults) {
        if (v)
            while (*v)
                out(*v++);
    } else {
        unsigned size = detail::StrLen(v);
        ToStrHelper(std::forward<OutF>(out), v, size, flags);
    }
}

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string<C> const & v, Flags flags) {
    if (0 == flags.has_any_non_defaults) {
        for (auto const c : v)
            out(c);
    } else {
        ToStrHelper(std::forward<OutF>(out), v.data(), unsigned(v.size()), flags);
    }
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string_view<C> const & v, Flags flags) {
    if (0 == flags.has_any_non_defaults) {
        for (auto const c : v)
            out(c);
    } else {
        ToStrHelper(std::forward<OutF>(out), v.data(), unsigned(v.size()), flags);
    }
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

//----------------------------------------------------------------------
// Handle float types:
//----------------------------------------------------------------------

template <typename OutF, typename RealT>
std::enable_if_t<
    std::is_same_v<RealT, float> ||
    std::is_same_v<RealT, double> ||
    std::is_same_v<RealT, long double>>
ToStr (OutF && out, RealT v, Flags flags) {
    char buffer [Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE];
    std::to_chars_result res;

    if (0 == flags.has_any_non_defaults) {
        res = std::to_chars(buffer, buffer + sizeof(buffer), v);
        for (char const * p = buffer; p != res.ptr; ++p)
            out(*p);
    } else {
        //bool is_negative = false;
        //if (flags.overridden.sign && v < 0) {
        //    v = -v;
        //    is_negative = true;
        //}
        std::chars_format fmt = ((flags.overridden.base && flags.base == 16)
            ? std::chars_format::hex
            : std::chars_format::fixed);
    
        if (flags.overridden.precision)
            res = std::to_chars(buffer, buffer + sizeof(buffer), v, fmt/*, flags.precision*/);  // FIXME(yzt): VS2017 still doesn't support this. 2019 isn't much better either.
        else
            res = std::to_chars(buffer, buffer + sizeof(buffer), v, fmt);

        if (flags.overridden.width || flags.overridden.fill || flags.overridden.justify /*|| flags.overridden.sign*/)
            ToStrHelper(std::forward<OutF>(out), buffer, unsigned(res.ptr - buffer), flags/*, true, is_negative*/);
        else
            for (char const * p = buffer; p != res.ptr; ++p)
                out(*p);
    }
}

//----------------------------------------------------------------------
// Handle integer types:
//----------------------------------------------------------------------

template <typename OutF, typename IntT>
std::enable_if_t<
//    std::is_same_v<IntT, signed char> ||
    std::is_same_v<IntT, signed short int> ||
    std::is_same_v<IntT, signed int> ||
    std::is_same_v<IntT, signed long int> ||
    std::is_same_v<IntT, signed long long int> ||
//    std::is_same_v<IntT, unsigned char> ||
    std::is_same_v<IntT, unsigned short int> ||
    std::is_same_v<IntT, unsigned int> ||
    std::is_same_v<IntT, unsigned long int> ||
    std::is_same_v<IntT, unsigned long long int>>
ToStr (OutF && out, IntT v, Flags flags) {
    char buffer [Y_OPT_FMT_MAX_INTEGER_NUM_CHAR_SIZE];
    std::to_chars_result res;

    if (0 == flags.has_any_non_defaults) {
        res = std::to_chars(buffer, buffer + sizeof(buffer), v);
        for (char const * p = buffer; p != res.ptr; ++p)
            out(*p);
    } else {
        int base = (flags.overridden.base ? flags.base : 10);
        if (base < 2) base = 2;
        if (base > 36) base = 36;
        res = std::to_chars(buffer, buffer + sizeof(buffer), v, base);

        if (flags.overridden.width || flags.overridden.fill || flags.overridden.justify /*|| flags.overridden.sign*/)
            ToStrHelper(std::forward<OutF>(out), buffer, unsigned(res.ptr - buffer), flags/*, true, is_negative*/);
        else
            for (char const * p = buffer; p != res.ptr; ++p)
                out(*p);
    }
}

//----------------------------------------------------------------------
// Handle character types:
//----------------------------------------------------------------------

template <typename OutF/*, typename CharT*/>
//std::enable_if<
//    std::is_same_v<CharT, char32_t> ||
//    std::is_same_v<CharT, wchar_t> ||
//    std::is_same_v<CharT, char16_t> ||
//    std::is_same_v<CharT, char>>
void
ToStr (OutF && out, char v, Flags flags) {
    if (0 == flags.has_any_non_defaults) {
        out(v);
    } else {
        ToStrHelper(std::forward<OutF>(out), &v, 1, flags);
    }
}

//----------------------------------------------------------------------
// Handle bool:
//----------------------------------------------------------------------

template <typename OutF>
void ToStr (OutF && out, bool v, Flags flags) {
    ToStr(std::forward<OutF>(out), (v ? '1' : '0'), flags);
}

//----------------------------------------------------------------------

        }   // namespace cvt

//======================================================================

enum class Err {
    AfterOpenBrace, // Expected '}' or '{' after open brace...
    TooFewArgs,
    TooManyArgs,
    BadFormat,
};

//======================================================================

        namespace detail {

//----------------------------------------------------------------------

template <typename OutF, typename T>
inline bool EmitValue (OutF && out, T && v, cvt::Flags flags) {
    //if (0 == flags.has_any_non_defaults)
    //    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v));
    //else    // flags have something non-default in them...
    //    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v), flags);
    //return true;
    
    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v), flags);
    return true;
}

template <typename ErrF, typename OutF, typename InF>
inline bool EmitArg (unsigned /*idx*/, cvt::Flags flags, ErrF && err, OutF && out, InF && in) {
    err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in));
    return false;
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    case  1: return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1), flags);
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    case  1: return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1), flags);
    case  2: return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2), flags);
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    case  1: return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1), flags);
    case  2: return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2), flags);
    case  3: return EmitValue(std::forward<OutF>(out), std::forward<ArgType3>(arg3), flags);
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3, ArgType4 && arg4) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    case  1: return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1), flags);
    case  2: return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2), flags);
    case  3: return EmitValue(std::forward<OutF>(out), std::forward<ArgType3>(arg3), flags);
    case  4: return EmitValue(std::forward<OutF>(out), std::forward<ArgType4>(arg4), flags);
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}
template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ... ArgTypes>
inline bool EmitArg (unsigned idx, cvt::Flags flags, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3, ArgType4 && arg4, ArgType5 && arg5, ArgTypes && ... args) {
    switch (idx) {
    case  0: return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0), flags);
    case  1: return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1), flags);
    case  2: return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2), flags);
    case  3: return EmitValue(std::forward<OutF>(out), std::forward<ArgType3>(arg3), flags);
    case  4: return EmitValue(std::forward<OutF>(out), std::forward<ArgType4>(arg4), flags);
    case  5: return EmitValue(std::forward<OutF>(out), std::forward<ArgType5>(arg5), flags);
    default: return EmitArg(idx - 6, std::forward<ErrF>(err), std::forward<OutF>(out), std::forward<InF>(in), std::forward<ArgTypes>(args)...);
    }
}

//----------------------------------------------------------------------

template <typename InF>
cvt::Flags ReadCvtFlags (InF && in) {
    cvt::Flags ret = {};
    auto c = in();
    if ('{' == c) {
        ret.escaped_curly_brace = true;
        ret.valid = true;
        return ret;
    }
    while ('0' <= c && c <= '9') {
        ret.index = 10 * ret.index + (c - '0');
        ret.explicit_index = true;
        c = in();
    }
    if ('w' == c) {
        c = in();
        while ('0' <= c && c <= '9') {
            ret.width = 10 * ret.width + (c - '0');
            ret.overridden.width = true;
            c = in();
        }
    }
    if ('p' == c) {
        c = in();
        while ('0' <= c && c <= '9') {
            ret.precision = 10 * ret.precision + (c - '0');
            ret.overridden.precision = true;
            c = in();
        }
    }
    if ('f' == c) {
        c = in();
        ret.fill = c;
        ret.overridden.fill = true;
        c = in();
    }
    if ('b' == c) {
        c = in();
        while ('0' <= c && c <= '9') {
            ret.base = 10 * ret.base + (c - '0');
            ret.overridden.base = true;
            c = in();
        }
    }
    if ('+' == c || '_' == c) {
    //    ret.sign = ('+' == c) ? cvt::Sign::Always : cvt::Sign::LeaveRoom;
    //    ret.overridden.sign = true;
        c = in();
    }
    if ('c' == c || 'C' == c || 'r' == c) {
        ret.justify = ('c' == c) ? cvt::Justify::CenterLeft : (('C' == c) ? cvt::Justify::CenterRight : cvt::Justify::Right);
        ret.overridden.justify = true;
        c = in();
    }
    if ('U' == c) {
    //    ret.case_ = cvt::Case::Upper;
    //    ret.overridden.case_ = true;
        c = in();
    }
    if ('}' == c) {
        ret.valid = true;
    }
    return ret;
}

//----------------------------------------------------------------------
// InF: A callable thing that returns a character-like object each time
//      it is called. Should return 0 when the format string is finished.
// OutF: A callable thing that will be called with a single character-like
//      parameter that it should emit out to the output stream or whatever.
//      It should return a bool-like thing, and when "false", the formatting
//      operation will stop. Will be called with a 0 at the end of input
//      (or upon error.)
// ErrF: A callable thing that will be called with 3 parameters: an "Err"
//      for the error code, the "out" object and the "in" object (in case
//      it needs to extract some state from them, e.g. position, etc.) It
//      should return a boolean-like thing to signal whether we should continue
//      or not.
template <typename ErrF, typename OutF, typename InF, typename ... ArgTypes>
void Do (ErrF && err, OutF && out, InF && in, ArgTypes && ... args) {
    uint8_t cur_arg = 0;
    bool should_continue = true;
    while (should_continue) {
        auto c = in();
        if ('\0' == c) {
            should_continue = false;
        } else if ('{' == c) {
            auto flags = ReadCvtFlags(std::forward<InF>(in));
            if (flags.valid) {
                if (flags. escaped_curly_brace) {
                    should_continue = out('{');
                } else {
                    if (!flags.explicit_index) {
                        flags.index = cur_arg++;
                    }
                    should_continue = EmitArg(flags.index, flags, std::forward<ErrF>(err), std::forward<OutF>(out), std::forward<InF>(in), std::forward<ArgTypes>(args)...);
                }
            } else {
                should_continue = err(Err::BadFormat, std::forward<OutF>(out), std::forward<InF>(in));
            }
        } else {
            should_continue = out(c);
        }
    }
    //if (cur_arg != sizeof...(args))
    //    err(Err::TooManyArgs, std::forward<OutF>(out), std::forward<InF>(in));
}

//----------------------------------------------------------------------

template <typename OutF>
auto UTF8inator (OutF && out) {
    return [&] (auto c) {
        if (c < 0x80) {
            return out((unsigned char)(c));
        } else if (c < 0x800) {
            return out((unsigned char)(0b110'00000 | ((c >>  6) & 0b000'11111)))
                && out((unsigned char)(0b10'000000 | ((c >>  0) & 0b00'111111)));
        } else if (c < 0x10000) {
            return out((unsigned char)(0b1110'0000 | ((c >> 12) & 0b0000'1111)))
                && out((unsigned char)(0b10'000000 | ((c >>  6) & 0b00'111111)))
                && out((unsigned char)(0b10'000000 | ((c >>  0) & 0b00'111111))); 
        } else {    // if (c < 0x110000) {
            return out((unsigned char)(0b11110'000 | ((c >> 18) & 0b00000'111)))
                && out((unsigned char)(0b10'000000 | ((c >> 12) & 0b00'111111)))
                && out((unsigned char)(0b10'000000 | ((c >>  6) & 0b00'111111)))
                && out((unsigned char)(0b10'000000 | ((c >>  0) & 0b00'111111)));
        }
    };
}

//----------------------------------------------------------------------

        }   // namespace _detail

//======================================================================

template <typename ... ArgTypes>
unsigned ToCStr (char * buffer, unsigned size, char const * fmt, ArgTypes && ... args) {
    unsigned ret = 0;
    if (buffer && size && fmt) {
        size -= 1;
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){if (ret < size) buffer[ret++] = c; return ret <= size;},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
        buffer[ret++] = '\0';
    }
    return ret;
}

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename ... ArgTypes>
unsigned ToCStr (char * buffer, unsigned size, std::string const & fmt, ArgTypes && ... args) {
    return ToCStr(buffer, size, fmt.c_str(), std::forward<ArgTypes>(args));
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename ... ArgTypes>
unsigned ToCStr (char * buffer, unsigned size, std::string_view const & fmt, ArgTypes && ... args) {
    unsigned ret = 0;
    if (buffer && size && fmt) {
        size -= 1;
        unsigned idx = 0;
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){if (ret < size) buffer[ret++] = c; return ret <= size;},
            [&]{return (idx < fmt.size()) ? fmt[idx++] : '\0';},
            std::forward<ArgTypes>(args)...
        );
        buffer[ret++] = '\0';
    }
    return ret;
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STDIO)
template <typename ... ArgTypes>
unsigned ToFile (FILE * file, char const * fmt, ArgTypes && ... args) {
    unsigned ret = 0;
    if (file && fmt) {
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){return ::fputc(c, file) != EOF;},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
    }
    return ret;
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STDIO)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STDIO)
template <typename ... ArgTypes>
unsigned ToConsole (char const * fmt, ArgTypes && ... args) {
    return ToFile(stdout, fmt, std::forward<ArgTypes>(args)...);
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STDIO)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_IOSTREAM)
template <typename ... ArgTypes>
unsigned ToFile (std::ostream & os, char const * fmt, ArgTypes && ... args) {
    unsigned ret = 0;
    if (os && fmt) {
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){os.put(c); return bool(os);},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
    }
    return ret;
}
#endif  // defined(Y_OPT_FMT_SUPPORT_IOSTREAM)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename ... ArgTypes>
std::string ToStr (char const * fmt, ArgTypes && ... args) {
    std::string ret;
    if (fmt) {
    #if defined(Y_OPT_FMT_STD_STRING_OUTPUT_PRECALC_SIZE)
        size_t len = 0;
        char const * fmt_cpy = fmt;
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto){len++; return true;},
            [&]{return *fmt_cpy++;},
            std::forward<ArgTypes>(args)...
        );
        ret.reserve(len);
    #endif
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){ret += c; return true;},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
    }
    return ret;
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename ... ArgTypes>
std::string ToStr (std::string const & fmt, ArgTypes && ... args) {
    return ToStr(fmt.c_str(), std::forward<ArgTypes>(args)...);
}
#endif  // defined(Y_OPT_FMT_SUPPORT_STD_STRING)

//----------------------------------------------------------------------

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename ... ArgTypes>
std::string ToStr (std::string_view const & fmt, ArgTypes && ... args) {
    std::string ret;
    if (fmt) {
        size_t idx = 0;
    #if defined(Y_OPT_FMT_STD_STRING_OUTPUT_PRECALC_SIZE)
        size_t len = 0;
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto){len++;},
            [&]{return (idx < fmt.size()) ? fmt[idx++] : '\0';},
            std::forward<ArgTypes>(args)...
        );
        idx = 0;
    #endif
        ret.reserve(len);
        detail::Do(
            [](auto, auto, auto){return false;},
            [&](auto c){ret += c;},
            [&]{return (idx < fmt.size()) ? fmt[idx++] : '\0';},
            std::forward<ArgTypes>(args)...
        );
    }
    return ret;
}
#endif  // defined(Y_OPT_FMT_SUPPORT_IOSTREAM)

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//======================================================================
//----------------------------------------------------------------------
//======================================================================

    }   // namespace fmt
}   // namespace y

//======================================================================
