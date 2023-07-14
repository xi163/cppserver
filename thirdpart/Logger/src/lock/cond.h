#pragma once

#include "../Macro.h"

#include "mutex.h"

namespace utils {
	class Mutex;
	class Cond {
	public:
		Cond(Mutex& mutex);
		~Cond();
		void wait();
		void wait(double seconds);
		void notify();
		void notifyAll();
	private:
		Mutex& mutex_;
		pthread_cond_t cond_;
	};
}