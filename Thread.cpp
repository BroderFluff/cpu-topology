#include "Thread.h"

#ifdef __has_builtin
#define HAS_BUILTIN(x) __has_builtin(x)
#else
#define HAS_BUILTIN(x) 0
#endif

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

namespace sys {

static int countTrailingZeroes(unsigned int v) noexcept {
#if __has_builtin(__builtin_ctz)
    return __builtin_ctz(v);
#endif
}

Thread::~Thread() {
    if (handle) {
        join();
        //destroy();
        CloseHandle(handle);
    }

}

Thread& Thread::operator=(Thread &&rhs) noexcept {
    if (this != &rhs) {
        std::swap(func, rhs.func);
        std::swap(handle, rhs.handle);
    }
    
    return *this;
}

THREAD_ROUTINE_CALL Thread::threadRoutine(void* args) {
    static_cast<Thread*>(args)->call();
    return 0;
}

bool Thread::start(std::uint64_t affinityMask) noexcept {
#ifdef _MSC_VER
    handle = (HANDLE) _beginthreadex(nullptr, 0, threadRoutine, this, CREATE_SUSPENDED, nullptr);

    if (handle == 0) {
        return false;
    }

    SetThreadAffinityMask(handle, affinityMask);
    ResumeThread(handle);

    return !!handle;
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    cpu_set_t set;
    CPU_ZERO(&set);

    while (affinityMask) {
        const auto i = countTrailingZeroes(affinityMask);
        CPU_SET(i, &set);
        affinityMask ^= (1ULL << i);
    }
    
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &set);

    return pthread_create(&handle, &attr, threadRoutine, this) == 0;
#endif
}

bool Thread::join() const noexcept {
#ifdef _MSC_VER
    return WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0;
#else
    return pthread_join(handle, nullptr);
#endif
}

}
