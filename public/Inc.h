#ifndef INCLUDE_INC_H
#define INCLUDE_INC_H

#include "Logger/src/utils/utils.h"

#include "IncMuduo.h"

#include <google/protobuf/message.h>

#include "IPFinder.h"

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "MongoDB/MongoDBClient.h"
#include "RedisClient/RedisClient.h"
#include "RedisLock/redlock.h"
#include "game_tracemsg/TraceMsg.h"

#define REDISCLIENT muduo::ThreadLocalSingleton<RedisClient>::instance()

#endif