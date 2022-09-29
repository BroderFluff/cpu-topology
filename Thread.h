#pragma once
#ifndef SYS_THREAD_H
#define SYS_THREAD_H

#include <cstdint>
#include <memory>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

#ifndef BIT_CHECK
#define BIT_CHECK(val, bits) \
    (((val) & (bits)) == (bits))
#endif

namespace sys {

#ifdef _MSC_VER
struct ThreadAffinity {
	DWORD_PTR affinity{ 0 };
	ThreadAffinity(std::uint64_t mask) : affinity{ mask } {}
};
#else
/* struct ThreadAffinity {
	cpu_set_t *setp;
	std::size_t count;

	ThreadAffinity(std::uint64_t mask) {
		const auto num = std::__popcount(mask);
	}

	operator const cpu_set_t *() const noexcept { return &setp; }
}; */
#endif

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

	Thread& operator=(Thread &&other) noexcept ;

	bool start(std::uint64_t mask) noexcept;
	bool join() const noexcept;
	bool detatch() noexcept;
	void destroy();

	void call() { func->call(); }

private:
	std::unique_ptr<Func> func{ nullptr };
	NativeHandle handle {};
};

}

#endif // SYS_THREAD_H
