
#include "y_profiler.h"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>
using namespace std::chrono_literals;

//#if defined(_WIN32)
//
//#if !defined(_WIN32_WINNT)
//    #define _WIN32_WINNT    0x0501  // We need at least Windows XP for ConvertFiberToThread()...
//#endif
//
//#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
//    #error "You must define _WIN32_WINNT to something newer than Win 2000!"
//#endif

profiler_system_t g_profiler_sys = {};
static std::thread * g_thread = nullptr;
static std::atomic_bool g_should_quit = false;

static unsigned const SampleBufferThreshold = 1'048'576;
static unsigned const SampleBufferSize = 4 * SampleBufferThreshold;
static profiler_sample_t sample_buffer_1 [SampleBufferSize];
static profiler_sample_t sample_buffer_2 [SampleBufferSize];

static inline uint64_t
Profiler_Internal_RDTSC () {
#if defined(_WIN32)
    return __rdtsc();
#else
    #error "RDTSC and return it..."
#endif
}

static inline void *
Profiler_Internal_AtomicExchangePointer (void ** ptr, void * new_value) {
#if defined(_WIN32)
    #if defined(_WIN64)
        static_assert(sizeof(void *) == sizeof(int64_t));
        return (void *)_InterlockedExchange64((int64_t *)ptr, (int64_t)new_value);
    #else
        static_assert(sizeof(void *) == sizeof(int32_t));
        return (void *)_InterlockedExchange((int32_t *)ptr, (int32_t)new_value);
    #endif
#else
    #error "AtomicExhchange, and return the old value in *ptr"
#endif
}

static void _cdecl
Profiler_Internal_Quit (
) {
    if (g_thread) {
        g_should_quit = true;
        g_thread->join();
        delete g_thread;
        g_thread = nullptr;

        g_should_quit = false;
        g_profiler_sys = {};
    }
}

static void
Profiler_Internal_ProcessSamples (
    profiler_sample_t * sample_buffer,
    size_t sample_count
) {
}

static void
Profiler_Internal_ThreadFunc (
) {
    while (!g_should_quit) {
        std::this_thread::sleep_for(100ms);

        auto p = g_profiler_sys.next_sample;    // should be atomic...
        auto current_buffer = g_profiler_sys.current_buffer;
        auto threshold = g_profiler_sys.buffer_threshold;
        if (p >= current_buffer + threshold) {
            profiler_sample_t * next_buffer = nullptr;
            if (current_buffer == g_profiler_sys.buffer1)
                next_buffer = g_profiler_sys.buffer2;
            else
                next_buffer = g_profiler_sys.buffer1;
            g_profiler_sys.current_buffer = next_buffer;
            
            auto filled_samples_end =
                (profiler_sample_t *)Profiler_Internal_AtomicExchangePointer(
                    (void **)&g_profiler_sys.next_sample,
                    next_buffer
                );

            Profiler_Internal_ProcessSamples(
                current_buffer, filled_samples_end - current_buffer
            );
        }
    }
}

bool
Profiler_Init (
) {
    if (g_profiler_sys.init_count <= 0) {
        g_profiler_sys.init_count = 1;
        g_profiler_sys.init_time = Profiler_Internal_RDTSC();

        g_profiler_sys.next_sample = sample_buffer_1;
        g_profiler_sys.current_buffer = sample_buffer_1;
        g_profiler_sys.buffer1 = sample_buffer_1;
        g_profiler_sys.buffer2 = sample_buffer_2;

        g_profiler_sys.buffer_total_size = SampleBufferSize;
        g_profiler_sys.buffer_threshold = SampleBufferThreshold;

        ::atexit(Profiler_Internal_Quit);

        g_thread = new std::thread (Profiler_Internal_ThreadFunc);
    }

    return true;
}

// If you don't init the profiling system, this will ensure that it happens before main()
// It's OK to call Profiler_Init() multiple times.
static bool g_first_init_initiator = Profiler_Init();

//#endif  // !defined(_WIN32)
