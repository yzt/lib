#pragma once

#if !defined(Y_PROFILER_H_INCLUDE_GUARD_)
    #define  Y_PROFILER_H_INCLUDE_GUARD_

#include <stddef.h> // for size_t
#include <stdint.h>

#if !defined(DISABLE_PROFILING)
    #define YP_BLOCK_BEGIN(name)    Profiler_Internal_CollectSample(ProfilerEvent_BlockBegin, "" name, __FUNCTION__, __FILE__, __LINE__)
    #define YP_BLOCK_END(name)      Profiler_Internal_CollectSample(ProfilerEvent_BlockEnd, "" name, __FUNCTION__, __FILE__, __LINE__)

    #if defined(__cplusplus)
    #define YP_SCOPE(name)          ::_profiler_details::Scope _scope_profiler ("" name, __FUNCTION__, __FILE__, __LINE__)
    #endif
#else
    #define YP_BLOCK_BEGIN(name)    /**/
    #define YP_BLOCK_END(name)      /**/
    #if defined(__cplusplus)
    #define YP_SCOPE(name)          /**/
    #endif
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    ProfilerEvent_BlockBegin,
    ProfilerEvent_BlockEnd,
} profiler_event_e;

bool Profiler_Init ();
void Profiler_Internal_CollectSample (
    profiler_event_e event,
    char const * literal_context_name,
    //char const * literal_user_str,
    char const * literal_function_name,
    char const * literal_file_name,
    int line_number
);


#if defined(__cplusplus)
namespace _profiler_details {
    class Scope {
    public:
        explicit Scope (char const * name, char const * func, char const * file, int line)
            : m_name (name), m_func (func), m_file (file), m_line (line) {
            Profiler_Internal_CollectSample(ProfilerEvent_BlockBegin, m_name, m_func, m_file, m_line);
        }
        ~Scope () {
            Profiler_Internal_CollectSample(ProfilerEvent_BlockEnd, m_name, m_func, m_file, m_line);
        }
    private:
        char const * m_name;
        char const * m_func;
        char const * m_file;
        int m_line;
    };
}
#endif

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_PROFILER_H_INCLUDE_GUARD_
