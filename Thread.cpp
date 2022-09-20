#include "Thread.h"

namespace sys {

Thread::~Thread() {
    if (handle) {
        join();
        //destroy();
    }
}

bool Thread::start() noexcept {
#ifdef _MSC_VER
	handle = _beginthread((_beginthread_proc_type) threadRoutine, 0, this);
    return !!handle;
#else
    return pthread_create(&handle, nullptr, threadRoutine, this) == 0;
#endif
}

bool Thread::join() const noexcept {
    return pthread_join(handle, nullptr);
}

}
