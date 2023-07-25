#ifndef INCLUDE_MUTEX_H
#define INCLUDE_MUTEX_H

#include "Logger/src/Macro.h"

namespace utils {
	class Mutex {
	public:
		class Guard {
		public:
			Guard(Mutex& mutex) :mutex_(mutex) {
				mutex_.Lock();
			}
			~Guard() {
				mutex_.Unlock();
			}
		private:
			Mutex& mutex_;
		};
	public:
		void Lock() {
			mutex_.lock();
		}
		void Unlock() {
			mutex_.unlock();
		}
	private:
		std::mutex mutex_;
	};
}

#endif