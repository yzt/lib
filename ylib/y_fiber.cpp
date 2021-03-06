
#include "y_fiber.h"

#include <cstdint>  // uintptr_t
#include <cstdio>
#include <cstdlib>

#define FIBER_STRINGIZE_DO_(x)              #x
#define FIBER_STRINGIZE(x)                  FIBER_STRINGIZE_DO_(x)
#define FIBER_ASSERT(cond, sys, msg)                            \
    do {                                                        \
        if (!(cond))                                            \
            sys->assert_fail_cb(                                \
                FIBER_STRINGIZE(cond),                          \
                __FILE__,                                       \
                __LINE__,                                       \
                msg                                             \
            );                                                  \
    } while (false)                                             \
    /**/

static void * DefaultAlloc (fiber_size_t size) {
    return ::malloc(size);
}

static void DefaultFree (void * ptr, fiber_size_t /*size*/) {
    ::free(ptr);
}

static void DefaultAssertFail (char const * cond_str, char const * filename, int line_no, char const * msg) {
    ::fprintf(stderr, "\n[FIBER ASSERTION FAILED] in %s:%d\n\t%s\n%s%s\n", filename, line_no, cond_str, msg ? msg : "", msg ? "\n" : "");
    ::fflush(stderr);
    ::abort();
}

#if defined(_WIN32)

#if !defined(_WIN32_WINNT)
    #define _WIN32_WINNT    0x0501  // We need at least Windows XP for ConvertFiberToThread()...
#endif

#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0501)
    #error "You must define _WIN32_WINNT to something newer than Win 2000!"
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static_assert(sizeof(fiber_handle_t) == sizeof(LPVOID), "Win32 fiber handle is a LPVOID, which must be the same as our fiber handle.");

struct fiber_internal_t {
    void * user_data;
    fiber_proc_t proc;
    fiber_system_t * sys;
    LPVOID win32_handle;
};

bool Fiber_SysInit (
    fiber_system_t * out_sys,
    fiber_size_t default_stack_reserve_size,
    fiber_size_t default_stack_commit_size,
    fiber_mem_alloc_callback_t alloc_cb,
    fiber_mem_free_callback_t free_cb,
    fiber_assert_fail_callback_t assert_fail_cb
) {
    bool ret = false;
    if (out_sys) {
        *out_sys = {};
        out_sys->alloc_cb = &DefaultAlloc;
        out_sys->free_cb = &DefaultFree;
        out_sys->assert_fail_cb = &DefaultAssertFail;
        out_sys->default_stack_reserve_size = default_stack_reserve_size;
        out_sys->default_stack_commit_size = default_stack_commit_size;
        
        if (alloc_cb && free_cb) {
            out_sys->alloc_cb = alloc_cb;
            out_sys->free_cb = free_cb;
        }
        
        if (assert_fail_cb)
            out_sys->assert_fail_cb = assert_fail_cb;
            
        if (out_sys->default_stack_commit_size > out_sys->default_stack_reserve_size)
            out_sys->default_stack_commit_size = out_sys->default_stack_reserve_size;
            
        auto main_fiber = static_cast<fiber_internal_t *>(out_sys->alloc_cb(sizeof(fiber_internal_t)));
        if (main_fiber) {
            void * main_fiber_win32_handle = ::ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
            if (main_fiber_win32_handle) {
                main_fiber->user_data = nullptr;
                main_fiber->proc = nullptr;
                main_fiber->sys = out_sys;
                main_fiber->win32_handle = main_fiber_win32_handle;

                out_sys->main_fiber = main_fiber;
                ret = true;
            } else
                out_sys->free_cb(main_fiber, sizeof(fiber_internal_t));
        }
    }
    return ret;
}

bool Fiber_SysCleanup (
    fiber_system_t * sys
) {
    bool ret = false;
    if (sys) {
        FIBER_ASSERT(0 == sys->live_fiber_count, sys, "There are fibers still alive in the system...");

        if (sys->main_fiber) {
            FIBER_ASSERT(::GetCurrentFiber() == static_cast<fiber_internal_t *>(sys->main_fiber)->win32_handle, sys, "This function should be called from the same fiber and thread that initialized the system.");

            if (::ConvertFiberToThread()) {
                sys->free_cb(sys->main_fiber, sizeof(fiber_internal_t));
                *sys = {};
                ret = true;
            }
        }
    }
    return ret;
}

void InternalFiberProcWrapper (void * param_) {
    auto param = static_cast<fiber_internal_t *>(param_);
    if (param && param->proc && param->sys) {
        param->proc(param);
        FIBER_ASSERT(false, param->sys, "Shouldn't have reached this point! Note that you must not return from a fiber proc.");
    }
}

fiber_handle_t Fiber_Create (
    fiber_system_t * sys,
    fiber_proc_t fiber_proc,
    void * user_data, 
    fiber_size_t stack_reserve_size,        // Set to zero to get the default
    fiber_size_t stack_commit_size          // Set to zero to get the default
) {
    fiber_handle_t ret = nullptr;
    if (sys && fiber_proc) {
        if (0 == stack_reserve_size)
            stack_reserve_size = sys->default_stack_reserve_size;
        if (0 == stack_commit_size)
            stack_commit_size = sys->default_stack_commit_size;
        
        auto fiber = static_cast<fiber_internal_t *>(sys->alloc_cb(sizeof(fiber_internal_t)));
        if (fiber) {
            fiber->user_data = user_data;
            fiber->proc = fiber_proc;
            fiber->sys = sys;
            fiber->win32_handle = ::CreateFiberEx(stack_commit_size, stack_reserve_size, FIBER_FLAG_FLOAT_SWITCH, InternalFiberProcWrapper, fiber);
            if (fiber->win32_handle) {
                sys->live_fiber_count += 1;
                ret = fiber;
            } else {
                sys->free_cb(fiber, sizeof(*fiber));
            }
        }
    }
    return ret;
}

bool Fiber_Destroy (
    fiber_handle_t fiber
) {
    bool ret = false;
    if (fiber) {
        auto p = static_cast<fiber_internal_t *>(fiber);
        if (p->sys) {
            auto sys = p->sys;
            size_t sz = sizeof(fiber_internal_t);
            
            *p = {};    // Reduce the chance of silent accidental post-free dereferencing...
            sys->free_cb(p, sz);
            
            sys->live_fiber_count -= 1;
            ret = true;
        }
    }
    return ret;
}

bool Fiber_ContextSwitch (
    fiber_handle_t from,
    fiber_handle_t to
) {
    bool ret = false;
    if (from && to) {
        auto p = static_cast<fiber_internal_t *>(from);
        auto q = static_cast<fiber_internal_t *>(to);
        FIBER_ASSERT(p->sys == q->sys, p->sys, "Trying to switch to a fiber in a different fiber system!");
        
        ::SwitchToFiber(q->win32_handle);
    }
    return ret;
}

void * Fiber_GetUserData (
    fiber_handle_t fiber
) {
    return fiber
        ? static_cast<fiber_internal_t *>(fiber)->user_data
        : nullptr;
}


void * Fiber_GetNativeHandle (
    fiber_handle_t fiber
) {
    return fiber
        ? static_cast<fiber_internal_t *>(fiber)->win32_handle
        : nullptr;
}

void Fiber_ContextSwitch_Unchecked (
    fiber_handle_t to
) {
    ::SwitchToFiber(static_cast<fiber_internal_t *>(to)->win32_handle);
}

fiber_handle_t Fiber_GetMyHandle () {
    return ::GetFiberData();
}

void * Fiber_GetMyNativeHandle () {
    return ::GetCurrentFiber();
}

#else	// Hopefully (almost) POSIX!
// FIXME
#define _XOPEN_SOURCE
#include <ucontext.h>

struct fiber_internal_t {
    void * user_data;
    fiber_proc_t proc;
    fiber_system_t * sys;
    ucontext_t ctx;             // This is really large (~1KB) (on Linux, circa 2018.)
    alignas(64) char stack [];
};

bool
Fiber_SysInit (
    fiber_system_t * out_sys,
    size_t default_stack_reserve_size,
    size_t default_stack_commit_size,
    fiber_mem_alloc_callback_t alloc_cb,
    fiber_mem_free_callback_t free_cb,
    fiber_assert_fail_callback_t assert_fail_cb
) {
    bool ret = false;
    if (out_sys) {
        *out_sys = {};
        out_sys->alloc_cb = &DefaultAlloc;
        out_sys->free_cb = &DefaultFree;
        out_sys->assert_fail_cb = &DefaultAssertFail;
        out_sys->default_stack_reserve_size = default_stack_reserve_size;
        out_sys->default_stack_commit_size = default_stack_commit_size;
        
        if (alloc_cb && free_cb) {
            out_sys->alloc_cb = alloc_cb;
            out_sys->free_cb = free_cb;
        }
        
        if (assert_fail_cb)
            out_sys->assert_fail_cb = assert_fail_cb;
            
        if (out_sys->default_stack_commit_size > out_sys->default_stack_reserve_size)
            out_sys->default_stack_commit_size = out_sys->default_stack_reserve_size;
            
        auto main_fiber = static_cast<fiber_internal_t *>(out_sys->alloc_cb(sizeof(fiber_internal_t)));
        if (main_fiber) {
            if (-1 != ::getcontext(&main_fiber->ctx)) {
                main_fiber->user_data = nullptr;
                main_fiber->proc = nullptr;
                main_fiber->sys = out_sys;
                
                out_sys->main_fiber = main_fiber;
                ret = true;
            } else
                out_sys->free_cb(main_fiber, sizeof(fiber_internal_t));
        }
    }
    return ret;
}

bool
Fiber_SysCleanup (
    fiber_system_t * sys
) {
    bool ret = false;
    if (sys) {
        FIBER_ASSERT(0 == sys->live_fiber_count, sys, "There are fibers still alive in the system...");

        if (sys->main_fiber)
            sys->free_cb(sys->main_fiber, sizeof(fiber_internal_t));
        *sys = {};
        
        ret = true;
    }
    return ret;
}

void InternalFiberProcWrapper (int p0, int p1) {
    static_assert(sizeof(unsigned long long) == 2 * sizeof(int), "");
    
    auto u = (((unsigned long long)p1 << (8 * sizeof(int))) | unsigned(p0));
    auto param = static_cast<fiber_internal_t *>(reinterpret_cast<void *>(uintptr_t(u)));

    if (param && param->proc && param->sys) {
        param->proc(param);
        FIBER_ASSERT(
            false, param->sys,
            "Shouldn't have reached this point! Note that you must not return from a fiber proc."
        );
    }
}

void InternalMakeContext (ucontext_t * ctx, fiber_handle_t param1) {
    static_assert(sizeof(uintptr_t) == sizeof(param1), "");

    int p0 = 0, p1 = 0;
    if constexpr (sizeof(param1) > sizeof(int)) {
        static_assert(sizeof(param1) <= 2 * sizeof(int), "fiber_handle_t should fit in *at most* two ints; not more.");
        auto u = reinterpret_cast<uintptr_t>(param1);
        p0 = int((u >> 0) & int(~0));
        p1 = int((u >> (8 * sizeof(int))) & int(~0));
    } else {
        p0 = int(reinterpret_cast<uintptr_t>(param1));
    }
    
    ::makecontext(ctx, reinterpret_cast<void (*) ()>(InternalFiberProcWrapper), 2, p0, p1);
}

fiber_handle_t
Fiber_Create (
    fiber_system_t * sys,
    fiber_proc_t fiber_proc,
    void * user_data, 
    fiber_size_t stack_reserve_size,
    fiber_size_t /*stack_commit_size*/
) {
    fiber_handle_t ret = nullptr;
    if (sys && fiber_proc) {
        if (stack_reserve_size <= 0)
            stack_reserve_size = sys->default_stack_reserve_size;

        auto mem = sys->alloc_cb(sizeof(fiber_internal_t) + stack_reserve_size);
        if (mem) {
            auto p = static_cast<fiber_internal_t *>(mem);
            if (-1 != ::getcontext(&p->ctx)) {
                p->user_data = user_data;
                p->proc = fiber_proc;
                p->sys = sys;
                p->ctx.uc_stack.ss_sp = p->stack;
                p->ctx.uc_stack.ss_size = stack_reserve_size;
                p->ctx.uc_link = &static_cast<fiber_internal_t *>(sys->main_fiber)->ctx;
                
                InternalMakeContext(&p->ctx, p);

                sys->live_fiber_count += 1;
                ret = mem;
            } else {
                sys->free_cb(mem, sizeof(fiber_internal_t) + stack_reserve_size);
            }
        }
    }
    return ret;
}

bool Fiber_Destroy (
    fiber_handle_t fiber
) {
    bool ret = false;
    if (fiber) {
        auto p = static_cast<fiber_internal_t *>(fiber);
        if (p->sys) {
            auto sys = p->sys;
            size_t sz = sizeof(fiber_internal_t) + p->ctx.uc_stack.ss_size;
            
            *p = {};    // Reduce the chance of silent accidental post-free dereferencing...
            sys->free_cb(p, sz);
            
            sys->live_fiber_count -= 1;
            ret = true;
        }
    }
    return ret;
}


bool Fiber_ContextSwitch (
    fiber_handle_t from,
    fiber_handle_t to
) {
    bool ret = false;
    if (from && to) {
        auto p = static_cast<fiber_internal_t *>(from);
        auto q = static_cast<fiber_internal_t *>(to);
        FIBER_ASSERT(p->sys == q->sys, p->sys, "Trying to switch to a fiber in a different fiber system!");
        
        ::swapcontext(&p->ctx, &q->ctx);
    }
    return ret;
}

void * Fiber_GetUserData (
    fiber_handle_t fiber
) {
    return fiber
        ? static_cast<fiber_internal_t *>(fiber)->user_data
        : nullptr;
}


void * Fiber_GetNativeHandle (
    fiber_handle_t fiber
) {
    return fiber
        ? &(static_cast<fiber_internal_t *>(fiber)->ctx)
        : nullptr;
}

#endif  // !defined(_WIN32)
