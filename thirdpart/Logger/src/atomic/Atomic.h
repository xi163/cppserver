#ifndef INCLUDE_ATOMIC_H
#define INCLUDE_ATOMIC_H

#include "Logger/src/Macro.h"

#ifdef _windows_
#define atom_incr(i)			InterlockedIncrement((volatile long *)&i)
#define atom_incr64(i)			InterlockedIncrement64((volatile long long *)&i)
#define atom_decr(i)			InterlockedDecrement((volatile long *)&i)
#define atom_decr64(i)			InterlockedDecrement64((volatile long long *)&i) // return new value
#define atom_add(i,val)			InterlockedExchangeAdd((volatile long *)&i,(long)(val))
#define atom_add64(i,val)		InterlockedExchangeAdd64((volatile long long *)&i,(long long)(val))
#define atom_sub(i,val)			InterlockedExchangeAdd((volatile long *)&i,-(long)(val))
#define atom_sub64(i,val)		InterlockedExchangeAdd64((volatile long long *)&i,-(long long)(val))
#define atom_assign(i, val)		InterlockedExchange((volatile long *)&i, (long)(val))
#define atom_assign64(i,val)	InterlockedExchange64((volatile long long *)&i, (long long)(val)) // return old value
#define atom_compare_assign(i,val,cmp) InterlockedCompareExchange((volatile long *)&i, (long)(val), (long)(cmp))
#define atom_compare_assign64(i,val,cmp) InterlockedCompareExchange64((volatile long long *)&i, (long long)(val), (long long)(cmp))
#define atom_assign_pointer(i, val) InterlockedExchangePointer(( void * volatile *)&i, (void *)(val))
#define atom_compare_assign_pointer(i,val,cmp) InterlockedCompareExchangePointer((void * volatile *)&i, (void *)(val), (void *)(cmp))
#elif defined(_linux_)
#define atom_incr(i)			__sync_add_and_fetch(&i, 1)
#define atom_incr64(i)			__sync_add_and_fetch(&i, 1)
#define atom_decr(i)			__sync_add_and_fetch(&i, -1)
#define atom_decr64(i)			__sync_add_and_fetch(&i, -1)
#define atom_add(i,val)			__sync_add_and_fetch(&i, (val))
#define atom_add64(i,val)		__sync_add_and_fetch(&i, (val))
#define atom_sub(i,val)			__sync_add_and_fetch(&i, -(val))
#define atom_sub64(i,val)		__sync_add_and_fetch(&i, -(val))
#define atom_assign(i, val)		__sync_lock_test_and_set(&i, (val)) // __sync_val_compare_and_swap(&i, i, (val)) __sync_lock_release
#define atom_assign64(i,val)	__sync_lock_test_and_set(&i, (val)) // __sync_val_compare_and_swap(&i, i, (val)) __sync_lock_release
//#define atom_compare_assign(i,val,cmp) __sync_val_compare_and_swap(&i, cmp, (val))
//#define atom_compare_assign64(i,val,cmp) __sync_val_compare_and_swap(&i, cmp, (val))
//#define atom_assign_pointer(i, val) __sync_lock_test_and_set(&i, (val))
//#define atom_compare_assign_pointer(i,val,cmp) __sync_val_compare_and_swap(&i, cmp, (val))
#endif

//\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\\//\/\/\/
/* 原子锁 */
// class Atom
// {
// 	/* 空闲/占有 */
// 	enum _{ kIdle_ = 0, kLocked_ = 1, };
// public:
// 	class GuardLock
// 	{
// 	public:
// 		GuardLock(Atom& lock) :lock_(lock) {
// 			lock_.lock();
// 		}
// 		~GuardLock() {
// 			lock_.unlock();
// 		};
// 	private:
// 		Atom& lock_;
// 	};
// public:
// 	Atom() :atom_(kIdle_) {  }
// 	/* 原子加锁 */
// 	inline void lock() {
// 		do{
// 			xsleep(0);
// 		} while (!tryLock());
// 	}
// 	/* 释放锁 */
// 	inline void unlock() {
// 		do {
// 			tryUnLock();
// 		} while (0);
// 	}
// private:
// 		inline bool tryLock()
// 		{
// 			/* 如果之前空闲, 尝试抢占, 并返回之前状态
// 			 * 如果返回空闲, 说明刚好抢占成功, 否则说明之前已被占有 */
// 			 return kIdle_ == atom_compare_assign(atom_, kLocked_, kIdle_);
// 		}
// 		inline bool tryUnLock()
// 		{
// 			/* 谁占有谁释放 */
// 			return kLocked_ == atom_compare_assign(atom_, kIdle_, kLocked_);
// 		}
// private:
// 	volatile long atom_;
// };

#endif