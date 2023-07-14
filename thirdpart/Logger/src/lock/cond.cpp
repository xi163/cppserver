#include "cond.h"

namespace utils {

	Cond::Cond(Mutex& mutex)
		: mutex_(mutex) {
		::pthread_cond_init(&cond_, NULL);
	}

	Cond::~Cond() {
		::pthread_cond_destroy(&cond_);
	}

	void wait() {
		::pthread_cond_wait(&cond_, mutex_.Get());
	}

	void Cond::wait(double seconds) {
		timespec abstime;
#ifdef _windows_
		clock_gettime(0, &abstime);
#else
		//CLOCK_REALTIME / CLOCK_MONOTONIC / CLOCK_MONOTONIC_RAW
		clock_gettime(CLOCK_REALTIME, &abstime);
#endif
		const long long NanoSecPerSec = 1e9;
		long long nanosecs = (long long)(seconds * NanoSecPerSec);

		abstime.tv_sec += (time_t)(abstime.tv_nsec + nanosecs) / NanoSecPerSec;
		abstime.tv_nsec = (long)(abstime.tv_nsec + nanosecs) % NanoSecPerSec;

		::pthread_cond_timedwait(&cond_, mutex_.Get(), &abstime);
	}

	void Cond::notify() {
		::pthread_cond_signal(&cond_);
	}

	void Cond::notifyAll() {
		::pthread_cond_broadcast(&cond_);
	}
}