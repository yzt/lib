yLib
====

A collection of mostly unrelated, mostly standalone, mostly single header+source, everyday libraries. Quite limited today; might be expanded later.

It has these modules:


Fiber
-----

A Win32-like API for **Fiber**s (cooperative thread-like objects that you must schedule yourself.)

* In files `y_fiber.h` and `y_fiber.cpp`.
* The interface is C, the implementation requires C++17 (for a stupid use of `if constexpr`.)
* It works on Windows and Linux (and probably other POSIX systems, although it uses a technically-deprecated part of POSIX, namely `<ucontext.h>`.)
* It has very low overhead (~5 nanosecs CPU time per context switch; about 12-32 bytes memory per fiber.)
* Has customizable memory allocation (does exactly 1 alloc/dealloc per fiber create/destroy.)
* Has customizable error reporting (somewhat; the assertion failure callback can be user-defined.)
* On POSIX platforms, you should conform to the Win32 convention of never returning from the fiber function.


Lock-free Queue
--------------

A lock-free, ring-organized, fixed-size, MPMC FIFO.

* In `y_lockfree.hpp`.
* It's a template and requires C++11.
* Works on Windows (MSVC) and Linux (G++ and Clang) and probably any other platform with a standard C++11 compiler.
* Only uses standard facilities.
* *Very* simple implementation (as lock-free stuff go.)
* Works with PODs, as well as non-trivial types.
* Supports multiple producers and multiple consumers.
* Does *not* allocate any memory; works within the memory you give it at construction.
* Verified with thread sanitizer (and other Clang/GCC sanitizers,) but no comprehensive testing has been done; use with caution!
* Is quite fast, but like most other lock-free data structures, its performance degrades under contention. For example, in my simple benchmarks on a 4GHz Intel Skylake CPU, the `put()` and `get()` methods took ~80ns with 1 producer and 1 cosumer, but degraded to ~2us with 10 producers and 10 consumers.
* The length of the queue *must* be a power of two and less than 2^16 (at most 32'768.) I might be able to support 65'536, but the need hasn't come up yet.
