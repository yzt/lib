#pragma once

//======================================================================

#define Y_OPT_FMT_SUPPORT_STDIO                     1
#define Y_OPT_FMT_SUPPORT_IOSTREAM                  1
#define Y_OPT_FMT_SUPPORT_STD_STRING                1
#define Y_OPT_FMT_STD_STRING_OUTPUT_PRECALC_SIZE    1
#define Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE            100

//======================================================================

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
 *      '{' arg_num [':' width] ['.' precision] [',' base] ['-' pad_char] [justify] [case] [sign] '}'
 *
 *      "arg_num"is from 0 onwards, the number of the argument that will be put in place of this format spec
 *      "width" is in [0..254], the minimum width of the whole field
 *      "precision" is in [0..254], the number of digits after decimal
 *      "base" is in [2..36]
 *      "pad_char" is a single character that should be used for padding on right-aligned values. Default is ' '.
 *      "justify" is exactly one of {'c', 'C', 'r'} for center-left, center-right, and right. Default is left-justified.
 *      "case" is 'U', which asks for upper-case stuff (hex digits, the 'e' in scientific representation, etc.) Default is lower-case.
 *      "sign" is one of {'+', '_'}, for always displaying the sign, or displaying either '-' or a space. Default is to only emit '-' for negative numbers.
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

enum class Justification : uint8_t {
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
            bool base : 1;
            bool padding : 1;
            bool justification : 1;
            bool case_ : 1;
            bool sign : 1;
        } overridden;
        uint8_t has_non_defaults;    // if non-zero
    };
    uint8_t width;
    uint8_t precision;
    uint8_t base;
    char pad;
    Justification justify;
    Case case_;
    Sign sign;
};
static_assert(sizeof(Flags) == 8, "");

//template <typename OutF, typename T>
//void ToStr (OutF && out, T const & v, Flags flags);

//----------------------------------------------------------------------

template <typename OutF>
void ToStr (OutF && out, char const * v) {
    if (v) {
        while (*v)
            out(*v++);
    } else {
        ToStr(std::forward<OutF>(out), "(null)");
    }
}

template <typename OutF>
void ToStr (OutF && out, char const * v, Flags flags) {
    if (v) {
        if (0 == flags.has_non_defaults) {
            while (*v)
                out(*v++);
        } else {    // the fun begins!
            while (*v)
                out(*v++);
        }
    } else {
        ToStr(std::forward<OutF>(out), "(null)", flags);
    }
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
    auto res = std::to_chars(buffer, buffer + sizeof(buffer), int(v));
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
//----------------------------------------------------------------------

        }   // namespace cvt

//======================================================================

enum class Err {
    AfterOpenBrace, // Expected '}' or '{' after open brace...
    TooFewArgs,
    TooManyArgs,
};

//======================================================================

        namespace _detail {

//----------------------------------------------------------------------

//template <typename OutF, typename T>
//inline bool EmitValue (OutF && out, T && v) {
//    //for (auto c : "[arg]"s)
//    //    out(c);
//    //return true;
//    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v)/*, {}*/);
//    return true;
//}

//----------------------------------------------------------------------

template <typename ErrF, typename OutF, typename InF>
inline bool EmitArg (unsigned /*idx*/, ErrF && err, OutF && out, InF && in) {
    err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in));
    return false;
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    case  1: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType1>(arg1)); return true;
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    case  1: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType1>(arg1)); return true;
    case  2: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType2>(arg2)); return true;
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    case  1: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType1>(arg1)); return true;
    case  2: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType2>(arg2)); return true;
    case  3: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType3>(arg3)); return true;
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3, ArgType4 && arg4) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    case  1: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType1>(arg1)); return true;
    case  2: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType2>(arg2)); return true;
    case  3: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType3>(arg3)); return true;
    case  4: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType4>(arg4)); return true;
    default: err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;
    }
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3, typename ArgType4, typename ArgType5, typename ... ArgTypes>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType2 && arg2, ArgType3 && arg3, ArgType4 && arg4, ArgType5 && arg5, ArgTypes && ... args) {
    switch (idx) {
    case  0: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType0>(arg0)); return true;
    case  1: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType1>(arg1)); return true;
    case  2: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType2>(arg2)); return true;
    case  3: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType3>(arg3)); return true;
    case  4: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType4>(arg4)); return true;
    case  5: cvt::ToStr(std::forward<OutF>(out), std::forward<ArgType5>(arg5)); return true;
    default: return EmitArg(idx - 6, std::forward<ErrF>(err), std::forward<OutF>(out), std::forward<InF>(in), std::forward<ArgTypes>(args)...);
    }
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
//      it needs to extract some state from them, e.g. position, etc.)
template <typename ErrF, typename OutF, typename InF, typename ... ArgTypes>
void Do (ErrF && err, OutF && out, InF && in, ArgTypes && ... args) {
    int cur_arg = 0;
    for (;;) {
        auto c = in();
        if ('\0' == c) {
            break;
        } else if ('{' == c) {
            c = in();
            if ('}' == c) {
                if (!EmitArg(cur_arg++, std::forward<ErrF>(err), std::forward<OutF>(out), std::forward<InF>(in), std::forward<ArgTypes>(args)...))
                    break;
            } else if ('{' == c) {
                out(c);
            } else {
                err(Err::AfterOpenBrace, out, in);
                break;
            }
        } else {
            if (!out(c))
                break;
        }
    }
    if (cur_arg != sizeof...(args))
        err(Err::TooManyArgs, std::forward<OutF>(out), std::forward<InF>(in));
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
        _detail::Do(
            [](auto, auto, auto){},
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
        _detail::Do(
            [](auto, auto, auto){},
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
        _detail::Do(
            [](auto, auto, auto){},
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
        _detail::Do(
            [](auto, auto, auto){},
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
        _detail::Do(
            [](auto, auto, auto){},
            [&](auto){len++;},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
    #endif
        ret.reserve(len);
        _detail::Do(
            [](auto, auto, auto){},
            [&](auto c){ret += c;},
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
        _detail::Do(
            [](auto, auto, auto){},
            [&](auto){len++;},
            [&]{return (idx < fmt.size()) ? fmt[idx++] : '\0';},
            std::forward<ArgTypes>(args)...
        );
        idx = 0;
    #endif
        ret.reserve(len);
        _detail::Do(
            [](auto, auto, auto){},
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
