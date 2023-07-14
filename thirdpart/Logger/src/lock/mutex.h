#pragma once

#include "../Macro.h"

#ifdef _windows_
#include "../pthread.h"
#include "../sched.h"
#include "../semaphore.h"
#endif

namespace utils {

//#if defined(_windows_)
	//typedef CRITICAL_SECTION mutex_t;
//#elif defined(_linux_)
	typedef pthread_mutex_t  mutex_t;
//#endif

	//Mutex
	class Mutex {
	public:
		Mutex();
		~Mutex();
		bool Try();
		void Lock();
		void Unlock();
		mutex_t& Get();
	private:
		mutex_t mutex_;
	};

	//GuardLock
	class GuardLock {
	public:
		GuardLock(Mutex& mutex) :mutex_(mutex) {
			mutex_.Lock();
		}
		~GuardLock() {
			mutex_.Unlock();
		};
	private:
		Mutex& mutex_;
	};

	//GuardUnLock
	class GuardUnLock {
	public:
		GuardUnLock(Mutex& mutex) :mutex_(mutex) {
			mutex_.Unlock();
		}
		~GuardUnLock() {
			mutex_.Lock();
		};
	private:
		Mutex& mutex_;
	};
}