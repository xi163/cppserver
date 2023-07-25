#ifndef INCLUDE_SPINLOCKIMPL_H
#define INCLUDE_SPINLOCKIMPL_H

#include "../../Macro.h"

#include <mutex>
//#include <shared_mutex>
#include <thread>
#include <condition_variable>

namespace utils {

	class SpinLockImpl {
	public:
		SpinLockImpl() {
		}
		SpinLockImpl(int timeout)
			: timeout_(timeout) {
		}
		void wait() {
			{
				std::unique_lock<std::mutex> lock(mutex_); {
					while (!sync_) {
						if (cond_.wait_for(lock, std::chrono::seconds(timeout_)) == std::cv_status::timeout) {
							break;
						}
					}
					sync_ = false;
				}
			}
		}
		void wait(int timeout) {
			{
				std::unique_lock<std::mutex> lock(mutex_); {
					while (!sync_) {
						if (cond_.wait_for(lock, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
							break;
						}
					}
					sync_ = false;
				}
			}
		}
		void notify() {
			{
				std::unique_lock<std::mutex> lock(mutex_); {
					sync_ = true;
					cond_.notify_one();
				}
				//std::this_thread::yield();
			}
		}
	private:
		int timeout_;
		bool sync_ = false;
		std::mutex mutex_;
		std::condition_variable cond_;
	};
}

#endif