#pragma once

//======================================================================

#define Y_OPT_FMT_SUPPORT_STD_STRING
#define Y_OPT_FMT_MAX_REAL_NUM_CHAR_SIZE    100

//======================================================================

#include <utility>  // std::forward
#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
    #include <string>
    #include <string_view>
#endif
#include <charconv> // std::to_chars() for float/double/long double

//======================================================================
/* The format string has the following format:
 *
 *
 */
//======================================================================

namespace y {
    namespace fmt {

//======================================================================

        namespace cvt {

//----------------------------------------------------------------------
//----------------------------------------------------------------------

struct Flags {
};

//template <typename OutF, typename T>
//void ToStr (OutF && out, T const & v, Flags flags);

//----------------------------------------------------------------------

template <typename OutF>
void ToStr (OutF && out, char const * v/*, Flags flags*/) {
    if (v) {
        while (*v)
            out(*v++);
    } else {
        out('('); out('n'); out('u'); out('l'); out('l'); out(')');
    }
}

#if defined(Y_OPT_FMT_SUPPORT_STD_STRING)
template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string<C> const & v) {
    for (auto c : v)
        out(c);
}

template <typename OutF, typename C>
void ToStr (OutF && out, std::basic_string_view<C> const & v) {
    for (auto c : v)
        out(c);
}
#endif

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

//----------------------------------------------------------------------
//----------------------------------------------------------------------

        }   // namespace cvt

//======================================================================

enum class Err {
    AfterOpenBrace, // Expected '}' or '{' after open brace...
    TooFewArgs,
};

//======================================================================


//----------------------------------------------------------------------

template <typename OutF, typename T>
inline bool EmitValue (OutF && out, T && v) {
    //for (auto c : "[arg]"s)
    //    out(c);
    //return true;
    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v)/*, {}*/);
    return true;
}

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
}

//======================================================================

template <typename ... ArgTypes>
unsigned c2c (char * buffer, unsigned size, char const * fmt, ArgTypes && ... args) {
    unsigned ret = 0;
    if (buffer && size && fmt) {
        size -= 1;
        Do(
            [](auto, auto, auto){},
            [&](char c){if (ret < size) buffer[ret++] = c; return ret <= size;},
            [&]{return *fmt++;},
            std::forward<ArgTypes>(args)...
        );
        buffer[ret++] = '\0';
    }
    return ret;
}

//======================================================================
//----------------------------------------------------------------------
//======================================================================

    }   // namespace fmt
}   // namespace y

//======================================================================
