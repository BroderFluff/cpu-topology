#pragma once
#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <memory>

#include <process.h>

namespace sys {

struct Func {
	virtual void call() noexcept = 0;
};

template <class F>
struct ThreadFunc : public Func {
	F func;
	ThreadFunc(F&& func) noexcept : func(std::move(func)) {}

	void call() noexcept override {
		func();
	}
};

class Thread {
public:
	Thread() = default;
	template <class F>
	Thread(F&& func) 
		: func{ std::make_unique<ThreadFunc<F>>(std::move(func)) } {
	}

	~Thread();

	void start() noexcept;

	void call() { func->call(); }

private:
	std::unique_ptr<Func> func{ nullptr };
};

inline void* threadRoutine(void* args) {
	static_cast<Thread*>(args)->call();
	return 0;
}

void Thread::start() noexcept {
	_beginthread((_beginthread_proc_type) threadRoutine, 0, this);
}

}

#endif // SYS_THREAD_H
