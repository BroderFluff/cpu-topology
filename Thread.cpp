#include "Thread.h"

#ifdef _MSC_VER
#define THREAD_ROUTINE_CALL void __cdecl
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
}

bool Thread::start() noexcept {
#ifdef _MSC_VER
    handle = (HANDLE) _beginthread(threadRoutine, 0, this);
    return !!handle;
#else
    return pthread_create(&handle, nullptr, threadRoutine, this) == 0;
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
