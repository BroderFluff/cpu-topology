#include "Thread.h"

#ifdef _MSC_VER
#define THREAD_ROUTINE_CALL unsigned __cdecl
#else
#define THREAD_ROUTINE_CALL void *
#endif

namespace sys {

Thread::~Thread() {
    if (handle) {
        join();
        //destroy();
    }
}

Thread& Thread::operator=(Thread &&other) noexcept {
    std::swap(func, other.func);
    std::swap(handle, other.handle);
    
    return *this;
}

static THREAD_ROUTINE_CALL threadRoutine(void* args) {
    static_cast<Thread*>(args)->call();

    return 0;
}

bool Thread::start(std::uint64_t affinityMask) noexcept {
#ifdef _MSC_VER
    handle = (HANDLE) _beginthreadex(nullptr, 0, threadRoutine, this, CREATE_SUSPENDED, nullptr);
    SetThreadAffinityMask(handle, affinityMask);
    ResumeThread(handle);
    //handle = (HANDLE) _beginthread(threadRoutine, 0, this);

    return !!handle;
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    cpu_set_t set;
    CPU_ZERO(&set);

    while (affinityMask) {
        const auto i = __builtin_ctz(affinityMask);
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
