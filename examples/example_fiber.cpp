#include <y_fiber.h>

#include <chrono>
#include <cstdio>

static constexpr int N = 10'000'000;

static double Now () {
    using namespace std::chrono;
    return duration_cast<duration<double>>(high_resolution_clock::now().time_since_epoch()).count();
}

static void FiberFunc1 (fiber_handle_t me) {
    auto next = reinterpret_cast<fiber_handle_t>(Fiber_GetUserData(me));
    printf("func1: started\n");
    printf("func1: context switch\n");
    Fiber_ContextSwitch(me, next);
    //printf("func1: returning\n");

    for (int i = 0; i < N; ++i)
        Fiber_ContextSwitch(me, next);
}

static void FiberFunc2 (fiber_handle_t me) {
    auto next = reinterpret_cast<fiber_handle_t>(Fiber_GetUserData(me));

    printf("func2: started\n");
    printf("func2: context switch\n");
    Fiber_ContextSwitch(me, next);
    //printf("func2: returning\n");

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

