#pragma once

#include <utility>  // std::forward

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
void ToStr (OutF && out, char const * v, Flags flags) {
    if (v) {
        while (*v)
            out(*v++);
    } else {
        out('('); out('n'); out('u'); out('l'); out('l'); out(')');
    }
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
    cvt::ToStr(std::forward<OutF>(out), std::forward<T>(v), {});
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
    if (0 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0));
    else {err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;}
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1) {
    if (0 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0));
    else if (1 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1));
    else {err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;}
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType1 && arg2) {
    if (0 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0));
    else if (1 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1));
    else if (2 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2));
    else {err(Err::TooFewArgs, std::forward<OutF>(out), std::forward<InF>(in)); return false;}
}

template <typename ErrF, typename OutF, typename InF, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3, typename ... ArgTypes>
inline bool EmitArg (unsigned idx, ErrF && err, OutF && out, InF && in, ArgType0 && arg0, ArgType1 && arg1, ArgType1 && arg2, ArgType1 && arg3, ArgTypes && ... args) {
    if (0 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType0>(arg0));
    else if (1 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType1>(arg1));
    else if (2 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg2));
    else if (3 == idx) return EmitValue(std::forward<OutF>(out), std::forward<ArgType2>(arg3));
    else return EmitArg(idx - 4, std::forward<ErrF>(err), std::forward<OutF>(out), std::forward<InF>(in), std::forward<ArgTypes>(args)...);
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
