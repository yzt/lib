#pragma once

#if !defined(Y_FIBER_H_INCLUDE_GUARD_)
    #define  Y_FIBER_H_INCLUDE_GUARD_

#include <stddef.h> // for size_t

#if defined(__cplusplus)
extern "C" {
#endif


typedef size_t fiber_size_t;
typedef void * fiber_handle_t;

typedef void (*fiber_proc_t) (fiber_handle_t me, void * user_data);
typedef void * (*fiber_mem_alloc_callback_t) (fiber_size_t size);
typedef void (*fiber_mem_free_callback_t) (void * ptr, fiber_size_t size);
typedef void (*fiber_assert_fail_callback_t) (
    char const * cond_str, char const * filename, int line_no, char const * msg
);

typedef struct {
    fiber_handle_t main_fiber;
    int live_fiber_count;
    fiber_mem_alloc_callback_t alloc_cb;
    fiber_mem_free_callback_t free_cb;
    fiber_assert_fail_callback_t assert_fail_cb;
    size_t default_stack_reserve_size;
    size_t default_stack_commit_size;
} fiber_system_t;



bool Fiber_SysInit (
    fiber_system_t * out_sys,
    fiber_size_t default_stack_reserve_size,
    fiber_size_t default_stack_commit_size,     // Should be in [0..reserve size].
    fiber_mem_alloc_callback_t alloc_cb,        // If either or both are NULL, will use
    fiber_mem_free_callback_t free_cb,          //  default malloc()-/free()-based versions.
    fiber_assert_fail_callback_t assert_fail_cb
);

bool Fiber_SysCleanup (
    fiber_system_t * sys
);


fiber_handle_t Fiber_Create (
    fiber_system_t * sys,
    fiber_proc_t fiber_proc,
    void * user_data, 
    fiber_size_t stack_reserve_size,        // Set to zero to get the default
    fiber_size_t stack_commit_size          // Set to zero to get the default
);

bool Fiber_Destroy (
    fiber_handle_t fiber
);


bool Fiber_ContextSwitch (
    fiber_handle_t from,
    fiber_handle_t to
);

void * Fiber_GetUserData (
    fiber_handle_t fiber
);

#if defined(__cplusplus)
}   // extern "C"
#endif

#endif  // Y_FIBER_H_INCLUDE_GUARD_
