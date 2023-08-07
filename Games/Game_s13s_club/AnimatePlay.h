#ifndef ANIMATEPLAY_INCLUDE_H
#define ANIMATEPLAY_INCLUDE_H

#include <chrono>

//////////////////////////////////////////////////////////////////////////
//动画播放时间 - 需要客户端配置
//////////////////////////////////////////////////////////////////////////
//#define StartGame						1   //开始游戏
//#define OpenCardCountDown				25  //开牌倒计时(理牌时间)
//#define OpenCard						3   //开牌动画
//#define Shoot                           2   //打枪/全垒打动画
//#define WinLost							3   //输赢飘金币动画
//#define NextCountDown                   3   //下一局倒计时

//////////////////////////////////////////////////////////////////////////
//定时器延迟时间
//////////////////////////////////////////////////////////////////////////
//理牌时间      开始游戏 + 开牌倒计时
//#define DELAY_GroupCard				(StartGame+OpenCardCountDown)
//开牌时间      开牌动画
//#define DELAY_OpenCard1					(OpenCard)
//#define DELAY_OpenCard2					(OpenCard+Shoot)
//游戏结算      输赢飘金币动画
//#define DELAY_WinLost					(WinLost)
//下一局
//#define DELAY_NextRound					(NextCountDown)

//////////////////////////////////////////////////////////////////////////
//动画播放时间线控制 AnimatePlay
// 1.当前动画播放类型 AnimateE
// 2.当前播放剩余时间 leftTime
//////////////////////////////////////////////////////////////////////////
struct AnimatePlay {

	enum StateE {
		GroupE,	  //理牌
		OpenE,	  //开牌
		PreEndE,  //结算
		NextE,	  //下一局
		MaxE,
	};
	//添加开始时间
	void add_START_time(StateE state, std::chrono::system_clock::time_point startTime) {
		startTime_[int(state)] = startTime;
	}
	//添加延迟时间
	void add_DELAY_time(StateE state, uint32_t delayTime) {
		delayTime_[int(state)] = delayTime;
	}
	//计算动画剩余播放时长
	//chrono::system_clock::now()
	double Get_LEFT_time(int status, std::chrono::system_clock::time_point now) {
		double leftTime = 0;
		//开始时间
		time_t startTime = Get_START_time(status);
		//延迟时间
		double delayTime = Get_DELAY_time(status);
		//当前时间
		time_t curTime = std::chrono::system_clock::to_time_t(now);
		//截止时间 = 开始时间 + 延迟时间
		//剩余时间 = 截止时间 - 当前时间
		leftTime = (double)startTime + delayTime - (double)curTime;
		return leftTime;
	}
	time_t Get_START_time(int status) {
		switch (status) {
		case GAME_STATUS_GROUP: return std::chrono::system_clock::to_time_t(startTime_[GroupE]);
		case GAME_STATUS_OPEN: return std::chrono::system_clock::to_time_t(startTime_[OpenE]);
		case GAME_STATUS_PREEND: return std::chrono::system_clock::to_time_t(startTime_[PreEndE]);
		case GAME_STATUS_NEXT: return std::chrono::system_clock::to_time_t(startTime_[NextE]);
		}
		return 0;
	}
	double Get_DELAY_time(int status) {
		switch (status) {
		case GAME_STATUS_GROUP: return delayTime_[GroupE];
		case GAME_STATUS_OPEN: return delayTime_[OpenE];
		case GAME_STATUS_PREEND: return delayTime_[PreEndE];
		case GAME_STATUS_NEXT: return delayTime_[NextE];
		}
		return 0;
	}
private:
	std::chrono::system_clock::time_point startTime_[MaxE];
	double delayTime_[MaxE];
};

#endif