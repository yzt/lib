#if 0

#include <ucontext.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>

static constexpr int N = 1'000'000;
static ucontext_t uctx_main, uctx_func1, uctx_func2;

static double Now () {
	using namespace std::chrono;
	return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

static void func1(void) {
	printf("func1: started\n");
	printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
	swapcontext(&uctx_func1, &uctx_func2);
	printf("func1: returning\n");

	for (int i = 0; i < N; ++i)
		::swapcontext(&uctx_func1, &uctx_main);
}

static void func2(void) {
	printf("func2: started\n");
	printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
	swapcontext(&uctx_func2, &uctx_func1);
	printf("func2: returning\n");

	for (int i = 0; i < N; ++i)
		::swapcontext(&uctx_func2, &uctx_func1);
}

int test_main () {
	char func1_stack[16384];
	char func2_stack[16384];

	getcontext(&uctx_func1);
	uctx_func1.uc_stack.ss_sp = func1_stack;
	uctx_func1.uc_stack.ss_size = sizeof(func1_stack);
	uctx_func1.uc_link = &uctx_main;
	makecontext(&uctx_func1, func1, 0);

	getcontext(&uctx_func2);
	uctx_func2.uc_stack.ss_sp = func2_stack;
	uctx_func2.uc_stack.ss_size = sizeof(func2_stack);
	uctx_func2.uc_link = &uctx_main;
	makecontext(&uctx_func2, func2, 0);

	printf("main: swapcontext(&uctx_main, &uctx_func2)\n");
	swapcontext(&uctx_main, &uctx_func2);

	printf("main: starting timing...\n");

	auto t0 = Now();
	for (int i = 0; i < N; ++i)
		::swapcontext(&uctx_main, &uctx_func2);
	auto dt = Now() - t0;
	::printf("main: timing done. %d context switches took %.3f secs (%.1f nanosecs per switch.)\n", 3 * N, dt, (dt * 1'000'000'000) / (3 * N));

	printf("main: exiting\n");
	return 0;
}
#endif

#include "y_fiber.h"

#include <chrono>
#include <cstdio>

static constexpr int N = 5'000'000;

static double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

static void FiberFunc1 (fiber_handle_t me, void * user_data) {
    auto next = reinterpret_cast<fiber_handle_t>(user_data);
    printf("func1: started\n");
    printf("func1: swapcontext(&uctx_func1, &uctx_func2)\n");
    Fiber_ContextSwitch(me, next);
    printf("func1: returning\n");

    for (int i = 0; i < N; ++i)
        Fiber_ContextSwitch(me, next);
}

static void FiberFunc2 (fiber_handle_t me, void * user_data) {
    auto next = reinterpret_cast<fiber_handle_t>(user_data);

    printf("func2: started\n");
    printf("func2: swapcontext(&uctx_func2, &uctx_func1)\n");
    Fiber_ContextSwitch(me, next);
    printf("func2: returning\n");

    for (int i = 0; i < N; ++i)
        Fiber_ContextSwitch(me, next);
}

int main () {
    fiber_system_t sys;
    if (false == Fiber_SysInit(&sys, 64 * 1024, 4 * 1024, nullptr, nullptr, nullptr)) {
        ::printf("[ERROR] Couldn't init fiber system.\n");
        return 1;
    }

    auto f1 = Fiber_Create(&sys, FiberFunc1, sys.main_fiber, 0, 0);
    auto f2 = Fiber_Create(&sys, FiberFunc2, f1, 0, 0);
    
    ::printf("main: Starting...\n");
    Fiber_ContextSwitch(sys.main_fiber, f2);
    ::printf("main: and we're back.\n");
    
	auto t0 = Now();
	for (int i = 0; i < N; ++i)
        Fiber_ContextSwitch(sys.main_fiber, f2);
	auto dt = Now() - t0;
	::printf("%d context switches took %.3f secs (%.1f nanosecs per switch.)\n", 3 * N, dt, (dt * 1'000'000'000) / (3 * N));

    Fiber_Destroy(f2);
    Fiber_Destroy(f1);
    if (false == Fiber_SysCleanup(&sys)) {
        ::printf("[ERROR] Couldn't clean up fiber system.\n");
        return 2;
    }
    return 0;
}

