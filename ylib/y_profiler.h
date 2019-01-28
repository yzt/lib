#pragma once

#if !defined(Y_PROFILER_H_INCLUDE_GUARD_)
    #define  Y_PROFILER_H_INCLUDE_GUARD_

#include <stddef.h> // for size_t
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    ProfilerEvent_BlockBegin,
    ProfilerEvent_BlockEnd,
} profiler_event_e;

typedef struct {
    uint32_t event;

    uint32_t thread_id;
    uint64_t timestamp;
    uint32_t core_id;

    uint32_t line_num;
    char const * file_name;
    char const * func_name;

    char const * ctx_name;
    char const * user_str;

    void * called_from;
} profiler_sample_t;

typedef struct {
    uint64_t total_overhead_cycles;
    uint64_t total_samples;
    profiler_sample_t * next_sample;

    profiler_sample_t * current_buffer;
    profiler_sample_t * buffer1;
    profiler_sample_t * buffer2;
    uint64_t buffer_total_size;
    uint64_t buffer_threshold;
    uint64_t init_time;
    int init_count;
} profiler_system_t;

extern profiler_system_t g_profiler_sys;


bool Profiler_Init (
    
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
    profiler_event_e event,
    char const * literal_context_name,
    char const * literal_user_str,
    char const * literal_function_name,
    char const * literal_file_name,
    int line_number
) {
    uint32_t core_id;
    uint64_t start_of_sampling = __rdtscp(&core_id);

#if defined(_WIN64)
    profiler_sample_t * sample = (profiler_sample_t *)_InterlockedExchangeAdd64(
        (int64_t *)&sys->next_sample, sizeof(profiler_sample_t)
    );
    uint32_t thread_id = __readgsdword(0x48);
#else
    profiler_sample_t * sample = (profiler_sample_t *)_InterlockedExchangeAdd(
        (int32_t *)&sys->current_sample, sizeof(profiler_sample_t)
    );
    uint32_t thread_id = __readfsdword(0x24);
#endif
    sample->event = event;
    sample->thread_id = thread_id;
    sample->timestamp = start_of_sampling;
    sample->core_id = core_id;
    sample->line_num = line_number;
    sample->file_name = literal_file_name;
    sample->func_name = literal_function_name;
    sample->ctx_name = literal_context_name;
    sample->user_str = literal_user_str;
    sample->called_from = _ReturnAddress();

    _InterlockedIncrement64((int64_t *)&sys->total_samples);
    uint64_t end_of_sampling = __rdtsc();
    _InterlockedExchangeAdd64((int64_t *)&sys->total_overhead_cycles, end_of_sampling - start_of_sampling);
}
#else
#error "Non-Windows platforms are not supported."
#endif

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_PROFILER_H_INCLUDE_GUARD_
