#include "mgoKeys.h"
#include "mgoOperation.h"
#include "RobotMgr.h"

static RobotDelegateCreator LoadLibrary(std::string const& serviceName) {
	std::string dllPath = boost::filesystem::initial_path<boost::filesystem::path>().string();
	dllPath.append("/");
	//./libGame_s13s_robot.so
	std::string dllName = utils::clearDllPrefix(serviceName);
	dllName.append("_robot");
	dllName.insert(0, "./lib");
	dllName.append(".so");
	dllName.insert(0, dllPath);
	Warnf(dllName.c_str());
	//getchar();
	void* handle = dlopen(dllName.c_str(), RTLD_LAZY);
	if (!handle) {
		Errorf("Can't Open %s, %s", dllName.c_str(), dlerror());
		exit(0);
	}
	RobotDelegateCreator creator = (RobotDelegateCreator)dlsym(handle, NameCreateRobotDelegate);
	if (!creator) {
		dlclose(handle);
		Errorf("Can't Find %s, %s", NameCreateRobotDelegate, dlerror());
		exit(0);
	}
	return creator;
}

CRobotMgr::CRobotMgr()
	: percentage_(0) {
}

CRobotMgr::~CRobotMgr() {
	freeItems_.clear();
	items_.clear();
}

bool CRobotMgr::Init(ITableContext* tableContext) {
	if (!tableContext->GetGameInfo() || !tableContext->GetRoomInfo()) {
		return false;
	}
	if (!tableContext->GetRoomInfo()->bEnableAndroid) {
		return false;
	}
	RobotDelegateCreator creator = LoadLibrary(tableContext->GetGameInfo()->serviceName);
	if (!creator) {
		exit(0);
	}
	return load(tableContext->GetRoomInfo(), tableContext, creator);
}

bool CRobotMgr::load(tagGameRoomInfo* roomInfo, ITableContext* tableContext, RobotDelegateCreator creator) {
	try {
		auto result = mgo::opt::FindOne(
			mgoKeys::db::GAMECONFIG,
			mgoKeys::tbl::ROBOT_STRATEGY,
			{},
			document{} << "gameid" << (int32_t)roomInfo->gameId << "roomid" << (int32_t)roomInfo->roomId << finalize);
		if (!result) {
			return false;
		}
		bsoncxx::document::view view = result->view();
		//Debugf(to_json(view).c_str());
		roomInfo->robotStrategy_.gameId = view["gameid"].get_int32();
		roomInfo->robotStrategy_.roomId = view["roomid"].get_int32();
		roomInfo->robotStrategy_.exitLowScore = view["exitLowScore"].get_int64();
		roomInfo->robotStrategy_.exitHighScore = view["exitHighScore"].get_int64();
		roomInfo->robotStrategy_.minScore = view["minScore"].get_int64();
		roomInfo->robotStrategy_.maxScore = view["maxScore"].get_int64();
		auto arr = view["areas"].get_array();
		for (auto& elem : arr.value) {
			AndroidStrategyArea area;
			area.weight = elem["weight"].get_int32();
			area.lowTimes = elem["lowTimes"].get_int32();
			area.highTimes = elem["highTimes"].get_int32();
			roomInfo->robotStrategy_.areas.emplace_back(area);
		}
		uint32_t robotCount = 0;
		//房间桌子数 * 每张桌子最大机器人数
		uint32_t needRobotCount = roomInfo->tableCount * roomInfo->maxAndroidCount;
		mongocxx::cursor cursor = mgo::opt::Find(
			mgoKeys::db::GAMECONFIG,
			mgoKeys::tbl::ROBOT_USER,
			{},
			document{} << "gameId" << (int32_t)roomInfo->gameId << "roomId" << (int32_t)roomInfo->roomId << finalize);
		for (auto& view : cursor) {
			//Warnf(to_json(view).c_str());
			UserBaseInfo userInfo;
			userInfo.userId = view["userId"].get_int64();
			userInfo.account = view["account"].get_utf8().value.to_string();
			userInfo.nickName = view["nickName"].get_utf8().value.to_string();
			userInfo.headId = view["headId"].get_int32();
			//view["enterTime"].get_utf8().value.to_string();
			//view["leaveTime"].get_utf8().value.to_string();
			//userInfo.takeMinScore = view["minScore"].get_int64();
			//userInfo.takeMaxScore = view["maxScore"].get_int64();
			userInfo.takeMinScore = roomInfo->robotStrategy_.minScore;
			userInfo.takeMaxScore = roomInfo->robotStrategy_.maxScore;
			userInfo.userScore = 0;
			userInfo.agentId = 0;
			userInfo.status = 0;
			userInfo.location = view["location"].get_utf8().value.to_string();
			//创建子游戏机器人代理
			std::shared_ptr<IRobotDelegate> robotDelegate = creator();
			//创建机器人
			std::shared_ptr<CRobot> robot(new CRobot());
			if (!robot || !robotDelegate) {
				Errorf("robot %d Failed", userInfo.userId);
				break;
			}
			robot->SetUserBaseInfo(userInfo);
			robotDelegate->SetStrategy(&roomInfo->robotStrategy_);
			robot->Init(robotDelegate);
			freeItems_.emplace_back(robot);
			if (++robotCount >= needRobotCount) {
				break;
			}
			Debugf("%d:%s %d:%s robotId:%d score:%ld",
				tableContext->GetGameInfo()->gameId,
				tableContext->GetGameInfo()->gameName.c_str(),
				tableContext->GetRoomInfo()->roomId,
				tableContext->GetRoomInfo()->roomName.c_str(),
				userInfo.userId, userInfo.userScore);
		}
		Warnf("%d:%s %d:%s tableCount:%d tableMaxRobotCount:%d robotCount:%d needRobotCount:%d",
			tableContext->GetGameInfo()->gameId,
			tableContext->GetGameInfo()->gameName.c_str(),
			tableContext->GetRoomInfo()->roomId,
			tableContext->GetRoomInfo()->roomName.c_str(),
			roomInfo->tableCount, roomInfo->maxAndroidCount, robotCount, needRobotCount);
		return true;
	}
	catch (const bsoncxx::exception& e) {
		Errorf(e.what());
		switch (mgo::opt::getErrCode(e.what())) {
		case 11000:
			break;
		default:
			break;
		}
	}
	catch (const std::exception& e) {
		Errorf(e.what());
	}
	catch (...) {
	}
	return false;
}

bool CRobotMgr::Empty() {
	{
		READ_LOCK(mutex_);
		return freeItems_.empty();
	}
}

std::shared_ptr<CRobot> CRobotMgr::Pick() {
	std::shared_ptr<CRobot> robot;
	{
		WRITE_LOCK(mutex_);
		if (!freeItems_.empty()) {
			robot = freeItems_.front();
			freeItems_.pop_front();
		}
	}
	if (robot) {
		robot->Reset();
		{
			WRITE_LOCK(mutex_);
			items_[robot->GetUserId()] = robot;
		}
	}
	return robot;
}

//一个userId占用多个CRobot对象?
void CRobotMgr::Delete(int64_t userId) {
	{
		WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CRobot>>::iterator it = items_.find(userId);
		if (it != items_.end()) {
			std::shared_ptr<CRobot> robot = it->second;
			items_.erase(it);
			robot->Reset();
			freeItems_.emplace_back(robot);
		}
	}
	Errorf("%d used = %d free = %d", userId, items_.size(), freeItems_.size());
}

void CRobotMgr::Delete(std::shared_ptr<CRobot> const& robot) {
	{
		WRITE_LOCK(mutex_);
		std::map<int64_t, std::shared_ptr<CRobot>>::iterator it = std::find_if(items_.begin(), items_.end(),
			[&](Item const& kv) -> bool {
				return kv.second.get() == robot.get();
			});
		if (it != items_.end()) {
			std::shared_ptr<CRobot>& robot = it->second;
			items_.erase(it);
			robot->Reset();
			freeItems_.emplace_back(robot);
		}
	}
	Errorf("%d used = %d free = %d", robot->GetUserId(), items_.size(), freeItems_.size());
}