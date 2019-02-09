
#include "y_profiler.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
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

#if !defined(_MSC_VER)
    #error "Only MSVC on Windows is supported. (Others *might* work though.)"
#endif
#include <intrin.h>

typedef struct {
    uint32_t event;

    uint32_t thread_id;
    uint64_t timestamp;
    uint32_t core_id;

    uint32_t line_num;
    char const * file_name;
    char const * func_name;

    char const * ctx_name;
    //char const * user_str;

    //void * called_from;
} profiler_sample_t;

// TODO(yzt): Add a global disable/pause flag.
typedef struct {
    uint64_t total_overhead_cycles;
    uint64_t total_samples;
    profiler_sample_t * next_sample;

    profiler_sample_t * current_buffer;
    profiler_sample_t * buffer1;
    profiler_sample_t * buffer2;
    uint64_t buffer_total_size;
    uint64_t buffer_threshold;
    double clock_freq;
    uint64_t init_time;
    uint64_t quit_time;
    int init_count;
} profiler_system_t;

//extern profiler_system_t g_profiler_sys;

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
#if defined(_WIN32)

/*/__forceinline/*/_declspec(noinline)/**/ void
Profiler_Internal_CollectSample (
    profiler_event_e event,
    char const * literal_context_name,
    //char const * literal_user_str,
    char const * literal_function_name,
    char const * literal_file_name,
    int line_number
) {
    uint32_t core_id;
    uint64_t start_of_sampling = __rdtscp(&core_id);

#if defined(_WIN64)
    profiler_sample_t * sample = (profiler_sample_t *)_InterlockedExchangeAdd64(
        (int64_t *)&g_profiler_sys.next_sample, sizeof(profiler_sample_t)
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
    //sample->user_str = literal_user_str;
    //sample->called_from = _ReturnAddress();   // FIXME(yzt): This is inlined, and _ReturnAddress is wrong.

    //// Calculating self overhead...
    //// Note(yzt): Takes too much time. Disabling for now.
    //_InterlockedIncrement64((int64_t *)&sys->total_samples);
    //uint64_t end_of_sampling = __rdtsc();
    //_InterlockedExchangeAdd64((int64_t *)&sys->total_overhead_cycles, end_of_sampling - start_of_sampling);
}
#else
#error "Non-Windows platforms are not supported."
#endif

static void
Profiler_Internal_Report () {
    auto const & p = g_profiler_sys;
    ::printf("[PROFILER] CPU clock freq: %0.2f MHz\n"
        , p.clock_freq / 1'000'000);
    ::printf("[PROFILER] Total run time: cycles= %llu, seconds= %0.6f\n"
        , p.quit_time - p.init_time, (p.quit_time - p.init_time) / p.clock_freq);
    ::printf("[PROFILER] Total overhead: cycles= %llu, seconds= %0.6f\n"
        , p.total_overhead_cycles, p.total_overhead_cycles / p.clock_freq);
    ::printf("[PROFILER] Total samples collected: %llu\n"
        , p.total_samples);
    ::printf("[PROFILER] Overhead per sample: cycles= %0.1f, nanoseconds= %0.6f\n"
        , p.total_overhead_cycles / double(p.total_samples), p.total_overhead_cycles / p.clock_freq * 1'000'000'000 / p.total_samples);
}

static void _cdecl
Profiler_Internal_Quit (
) {
    if (g_thread) {
        g_should_quit = true;
        g_thread->join();
        delete g_thread;
        g_thread = nullptr;

        g_profiler_sys.quit_time = Profiler_Internal_RDTSC();
        Profiler_Internal_Report();

        g_should_quit = false;
        g_profiler_sys = {};
    }
}

struct Entry {
    char const * ctx_name;
    uint64_t enter_count;
    uint64_t exit_count;
    uint64_t total_time;
    uint64_t last_enter_time;
    char const * func_name;
    char const * file_name;
    uint32_t line_num;
};

std::vector<Entry> g_entries;

static void
Profiler_Internal_ProcessSamples (
    profiler_sample_t * sample_buffer,
    size_t sample_count
) {
    auto p = sample_buffer;
    for (size_t i = 0; i < sample_count; ++i, ++p) {
        Entry * e = nullptr;
        for (unsigned j = 0; j < g_entries.size(); ++j)
            if (g_entries[j].ctx_name == p->ctx_name) {
                e = g_entries.data() + j;
                break;
            }
        if (!e) {
            g_entries.emplace_back();
            e = &(g_entries.back());
            e->ctx_name = p->ctx_name;
            e->enter_count = 0;
            e->exit_count = 0;
            e->total_time = 0;
            e->func_name = p->func_name;
            e->file_name = p->file_name;
            e->line_num = p->line_num;
        }
        assert(e->func_name == p->func_name);
        assert(e->file_name == p->file_name);
        assert(e->line_num == p->line_num);
        switch (p->event) {
        case ProfilerEvent_BlockBegin:
            assert(0);
            //...
            break;
        case ProfilerEvent_BlockEnd:
            break;
        }
    #if 0
        ::printf(
            "%d, %u, %llu, %u, %s(%p)"
            //", %s(%p)@%u"
            ", %s(%p)"
            //", 0x%p"
            "\n"
            , int(p->event)
            , p->thread_id, p->timestamp, p->core_id
            , p->ctx_name, p->ctx_name
            //, p->file_name, p->file_name, p->line_num
            , p->func_name, p->func_name
            //, p->called_from
        );
    #endif
    }
    //::printf("--\n");
}

static double
Profiler_Internal_CalcRDTSCFreq () {
    using namespace std::chrono;
    auto t0 = steady_clock::now();
    auto c0 = Profiler_Internal_RDTSC();
    std::this_thread::sleep_for(100ms);
    auto c1 = Profiler_Internal_RDTSC();
    auto t1 = steady_clock::now();

    auto dt = t1 - t0;
    uint64_t dc = c1 - c0;
    double clocks_per_sec = double(dc) / duration_cast<duration<double>>(dt).count();
    return clocks_per_sec;
}

static void
Profiler_Internal_ThreadFunc (
) {
    g_profiler_sys.clock_freq = Profiler_Internal_CalcRDTSCFreq();

    while (!g_should_quit) {
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

        if (g_should_quit)
            break;

        //std::this_thread::yield();
        std::this_thread::sleep_for(10ms);
    }

    // FIXME(yzt): make sure we aren't collectiong samples still!
    Profiler_Internal_ProcessSamples(
        g_profiler_sys.current_buffer,
        g_profiler_sys.next_sample - g_profiler_sys.current_buffer
    );
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

        #if !defined(DISABLE_PROFILING)
        g_thread = new std::thread (Profiler_Internal_ThreadFunc);
        #endif
    }

    return true;
}

// If you don't init the profiling system, this will ensure that it happens before main()
// It's OK to call Profiler_Init() multiple times.
static bool g_first_init_initiator = Profiler_Init();

//#endif  // !defined(_WIN32)
