
#include "y_profiler.h"

#include <cstdint>  // uintptr_t

#if defined(_WIN32)

#if !defined(_WIN32_WINNT)
    #define _WIN32_WINNT    0x0501  // We need at least Windows XP for ConvertFiberToThread()...
#endif

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
    #error "You must define _WIN32_WINNT to something newer than Win 2000!"
#endif


#endif  // !defined(_WIN32)
