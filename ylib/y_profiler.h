#pragma once

#if !defined(Y_PROFILER_H_INCLUDE_GUARD_)
    #define  Y_PROFILER_H_INCLUDE_GUARD_

#include <stddef.h> // for size_t
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    uint64_t timestamp;
    uint32_t core_id;
    uint32_t thread_id;

    char const * ctx_name;
    char const * user_str;

    char const * func_name;
    char const * file_name;
    uint32_t line_num;

    uint64_t code_address;
} profiler_sample_t;

typedef struct {
    profiler_sample_t * current_sample;
    profiler_sample_t * buffer1;
    profiler_sample_t * buffer2;
    uint64_t buffer_sample_count;
} profiler_system_t;



bool Profiler_SysInit (
    profiler_system_t * out_sys
);

bool Profiler_SysCleanup (
    profiler_system_t * sys
);

#if defined(_WIN32)
#if !defined(_MSC_VER)
    #error "Only MSVC is supported on Windows. (Others *might* work though.)"
#endif
#include <intrin.h>

//#pragma intrinsic(_InterlockedAdd64)

static __forceinline void
Profiler_Internal_CollectSample (
    profiler_system_t * sys,
    char const * literal_context_name,
    char const * literal_user_str,
    char const * literal_function_name,
    char const * literal_file_name,
    int line_number
) {
#if defined(_WIN64)
    profiler_sample_t * sample = (profiler_sample_t *)_InterlockedExchangeAdd64(
        (int64_t *)&sys->current_sample, sizeof(profiler_sample_t)
    );

#else
    profiler_sample_t * sample = (profiler_sample_t *)_InterlockedExchangeAdd(
        (int32_t *)&sys->current_sample, sizeof(profiler_sample_t)
    );
#endif
}
#else
#error "Non-Windows platforms are not supported."
#endif

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_PROFILER_H_INCLUDE_GUARD_
