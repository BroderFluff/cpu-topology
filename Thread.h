#pragma once
#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <memory>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

namespace sys {

struct Func {
					virtual ~Func() = default;
	virtual void	call() noexcept = 0;
};

template <class Fn>
struct ThreadFunc : public Func {
	Fn		func;

			ThreadFunc(Fn&& func) noexcept : func(std::move(func)) {}
	void	call() noexcept override { func(); }
};

class Thread final {
public:
#if defined(_MSC_VER)
using NativeHandle = HANDLE;
#else
using NativeHandle = pthread_t;
#endif
	Thread() = default;

	template <class Fn>
	Thread(Fn&& func) : func{ std::make_unique<ThreadFunc<Fn>>(std::move(func)) } {}
	~Thread();

	Thread& operator=(Thread &&other) noexcept {
		std::swap(func, other.func);
		std::swap(handle, other.handle);
		return *this;
	}

	bool start() noexcept;
	bool join() const noexcept;
	bool detatch() noexcept;
	void destroy();

	void call() { func->call(); }

private:
	std::unique_ptr<Func> func{ nullptr };
	NativeHandle handle;
};

}

#endif // SYS_THREAD_H
