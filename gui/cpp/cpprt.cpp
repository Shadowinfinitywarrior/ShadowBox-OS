// cpprt.cpp  —  Minimal C++ runtime for freestanding builds
//
// Provides operator new / operator delete that delegate to our
// freestanding malloc/free (defined in freestanding.c).
// No exceptions, no RTTI, no std::bad_alloc — a null is returned on failure.

#include <cstddef>

extern "C" {
    void* malloc(size_t);
    void  free  (void*);
}

// Placement new is defined inline in Compositor.cpp / InputRouter.cpp.
// Here we provide the global scalar new/delete.

void* operator new(size_t sz) noexcept {
    return ::malloc(sz);
}

void* operator new[](size_t sz) noexcept {
    return ::malloc(sz);
}

void operator delete(void* p) noexcept {
    ::free(p);
}

void operator delete[](void* p) noexcept {
    ::free(p);
}

// Sized deletes (C++14) — simply forward to free
void operator delete(void* p, size_t) noexcept {
    ::free(p);
}

void operator delete[](void* p, size_t) noexcept {
    ::free(p);
}

// Pure virtual call handler (required when -fno-rtti)
extern "C" void __cxa_pure_virtual() {
    // Trap — pure virtual should never be called in a correct program
    __asm__ volatile("int3");
    while (1) __asm__("hlt");
}

// __cxa_guard_* stubs (for static local initialization, single-threaded)
extern "C" {
    int  __cxa_guard_acquire(int* g) { return (*g == 0) ? 1 : 0; }
    void __cxa_guard_release(int* g) { *g  = 1; }
    void __cxa_guard_abort  (int* /*g*/) {}
    void __cxa_atexit_stub  ()       {}
}
