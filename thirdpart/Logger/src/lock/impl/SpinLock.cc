#include "SpinLockImpl.h"
#include "../SpinLock.h"
//#include "../../utils/impl/utilsImpl.h"
#include "../../auth/auth.h"

namespace utils {

	template <class T> static inline T* New() {
		void* ptr = (void*)malloc(sizeof(T));
		return new(ptr) T();
	}

	template <class T, typename Ty> static inline T* New(Ty v) {
		void* ptr = (void*)malloc(sizeof(T));
		return new(ptr) T(v);
	}

	template <class T> static inline void Delete(T* ptr) {
		ptr->~T();
		free(ptr);
	}

	SpinLock::SpinLock() :impl_(New<SpinLockImpl>()) {
		//placement new
	}

	SpinLock::SpinLock(int timeout) :impl_(New<SpinLockImpl>(timeout)) {
	}

	SpinLock::~SpinLock() {
		Delete<SpinLockImpl>(impl_);
	}

	void SpinLock::wait() {
		AUTHORIZATION_CHECK;
		impl_->wait();
	}

	void SpinLock::wait(int timeout) {
		AUTHORIZATION_CHECK;
		impl_->wait(timeout);
	}

	void SpinLock::notify() {
		AUTHORIZATION_CHECK;
		impl_->notify();
	}

}