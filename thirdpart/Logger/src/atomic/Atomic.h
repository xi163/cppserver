#ifndef INCLUDE_ATOMIC_H
#define INCLUDE_ATOMIC_H

#include "Logger/src/Macro.h"

#ifdef _windows_

#define incrementAndGet(dst)		InterlockedIncrement((long *)&dst)
#define decrementAndGet(dst)		InterlockedDecrement((long *)&dst)

#define addAndGet(dst, val)			InterlockedExchangeAdd((long *)&dst, (long)(val))
#define subAndGet(dst, val)			InterlockedExchangeAdd((long *)&dst, -(long)(val))

#define getAndSet(dst, val)		    InterlockedExchange((long *)&dst, (long)(val))
#define getAndSetPtr(dst, val)		InterlockedExchangePointer((void **)&dst, (void *)(val))

#define getCmpSet(dst, cmp, val)    InterlockedCompareExchange((long *)&dst, (long)(val), (long)(cmp))
#define getCmpSetPtr(dst, cmp, val) InterlockedCompareExchangePointer((void **)&dst, (void *)(val), (void *)(cmp))

#elif defined(_linux_)

#define getAndIncrement(dst)	    __sync_fetch_and_add(&dst, 1)
#define getAndDecrement(dst)	    __sync_fetch_and_add(&dst, -1)

#define getAndAdd(dst,val)		    __sync_fetch_and_add(&dst, val)
#define getAndSub(dst,val)		    __sync_fetch_and_add(&dst, -(val))

#define incrementAndGet(dst)	    __sync_add_and_fetch(&dst, 1)
#define decrementAndGet(dst)	    __sync_add_and_fetch(&dst, -1)

#define addAndGet(dst,val)		    __sync_add_and_fetch(&dst, val)
#define subAndGet(dst,val)		    __sync_add_and_fetch(&dst, -(val))

#define getAndSet(dst, val)		    __sync_lock_test_and_set(&dst, (val))
#define getAndSetPtr(dst, val)      __sync_lock_test_and_set(&dst, (val))

#define getCmpSet(dst, cmp, val)    __sync_val_compare_and_swap(&dst, cmp, (val))
#define getCmpSetPtr(dst, cmp, val) __sync_val_compare_and_swap(&dst, cmp, (val))

#endif

#define CAS(ptr, oldval, newval) getCmpSetPtr(ptr, oldval, newval) == (oldval)

//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\\//\/\/\/
/* 原子锁 */
// class AtomicLock {
// 	enum _ { kIdle_ = 0, kLocked_ = 1, };
// public:
// 	class GuardLock {
// 	public:
// 		GuardLock(AtomicLock& lock) :lock_(lock) {
// 			lock_.lock();
// 		}
// 		~GuardLock() {
// 			lock_.unlock();
// 		};
// 	private:
// 		AtomicLock& lock_;
// 	};
// public:
// 	AtomicLock() : val_(kIdle_) {  }
// 	inline void lock() {
// 		do {
// 			xsleep(0);
// 		} while (!tryLock());
// 	}
// 	inline void unlock() {
// 		do {
// 			tryUnLock();
// 		} while (0);
// 	}
// private:
// 	inline bool tryLock() {
// 		/* 如果之前空闲, 尝试抢占, 并返回之前状态
// 		 * 如果返回空闲, 说明刚好抢占成功, 否则说明之前已被占有 */
// 		return kIdle_ == getCmpSet(val_, kIdle_, kLocked_);
// 	}
// 	inline bool tryUnLock() {
// 		/* 谁占有谁释放 */
// 		return kLocked_ == getCmpSet(val_, kLocked_, kIdle_);
// 	}
// private:
// 	volatile long val_;
// };

#endif