#include "timer.h"
#include "../log/impl/LoggerImpl.h"

namespace utils {

	Timer::Timer() :expired_(true), try_to_expire_(false) {
	}

	Timer::Timer(const Timer& t) {
		expired_ = t.expired_.load();
		try_to_expire_ = t.try_to_expire_.load();
	}

	Timer::~Timer() {
		Expire();
	}

	void Timer::StartTimer(int interval, std::function<void()> task) {
		if (expired_ == false) {
			return;
		}
		expired_ = false;
		std::thread([this, interval, task]() {
			while (!try_to_expire_) {
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				task();
			}
			{
				std::lock_guard<std::mutex> locker(mutex_);
				expired_ = true;
				expired_cond_.notify_one();
			}
			}).detach();
	}

	void Timer::Expire() {
		if (expired_) {
			return;
		}
		if (try_to_expire_) {
			return;
		}
		try_to_expire_ = true; {
			std::unique_lock<std::mutex> locker(mutex_);
			expired_cond_.wait(locker, [this] {return expired_ == true; });
			if (expired_ == true) {
				try_to_expire_ = false;
			}
		}
	}

	void EchoFunc(std::string const&& s) {
		_Errorf_tmsp("test: %s", s.c_str());
	}

	void testTimer() {
		Timer t;
		//周期性执行定时任务
		t.StartTimer(10000, std::bind(EchoFunc, "timeout StartTimer1!"));
		//std::this_thread::sleep_for(std::chrono::seconds(4));
		_Warnf_tmsp("try to expire timer1!");
		t.Expire();

		//周期性执行定时任务
		//t.StartTimer(10000, std::bind(EchoFunc, "timeout StartTimer2!"));
		//std::this_thread::sleep_for(std::chrono::seconds(4));
		//_Infof_tmsp("try to expire timer2!");
		//t.Expire();

		//std::this_thread::sleep_for(std::chrono::seconds(2));

		//只执行一次定时任务
		//同步
		t.SyncWait(10000, EchoFunc, "timeout SyncWait!");
		//异步
		t.AsyncWait(10000, EchoFunc, "timeout AsyncWait!");

		//std::this_thread::sleep_for(std::chrono::seconds(10));
	}
}