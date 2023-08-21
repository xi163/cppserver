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

#define CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)
#define CALLBACK_4(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)
#define CALLBACK_5(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##__VA_ARGS__)

#define REDISCLIENT muduo::ThreadLocalSingleton<RedisClient>::instance()
#define MONGODBCLIENT MongoDBClient::ThreadLocalSingleton::instance()
#define REDISLOCK RedisLock::ThreadLocalSingleton::instance()

#endif