#ifndef INCLUDE_SPINLOCK_H
#define INCLUDE_SPINLOCK_H

#include "Logger/src/Macro.h"

namespace utils {

	class SpinLockImpl;
	class SpinLock {
	public:
		SpinLock();
		SpinLock(int timeout);
		~SpinLock();
		void wait();
		void wait(int timeout);
		void notify();
	private:
		SpinLockImpl* impl_;
	};
}

#endif