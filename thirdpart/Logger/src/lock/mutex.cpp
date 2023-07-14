#include "mutex.h"

#ifdef _windows_
#include <synchapi.h>
#endif

namespace utils {
	Mutex::Mutex() {
//#if defined(_windows_)
		//::InitializeCriticalSection(&mutex_);
		//::InitializeCriticalSectionAndSpinCount(&mutex_, 500);
//#elif defined(_linux_)
		::pthread_mutex_init(&mutex_, NULL);
//#endif
	}

	Mutex::~Mutex() {
//#if defined(_windows_)
		//::DeleteCriticalSection(&mutex_);
//#elif defined(_linux_)
		::pthread_mutex_destroy(&mutex_);
//#endif
	}

	mutex_t& Mutex::Get() {
		return mutex_;
	}

	bool Mutex::Try() {
//#if defined(_windows_)
		//return ::TryEnterCriticalSection(&mutex_);
//#elif defined(_linux_)
		return 0 == ::pthread_mutex_trylock(&mutex_);
//#endif
	}

	void Mutex::Lock() {
//#if defined(_windows_)
		//::EnterCriticalSection(&mutex_);
//#elif defined(_linux_)
		::pthread_mutex_lock(&mutex_);
//#endif
	}
	void Mutex::Unlock() {
//#if defined(_windows_)
		//::LeaveCriticalSection(&mutex_);
//#elif defined(_linux_)
		::pthread_mutex_unlock(&mutex_);
//#endif
	}
}