#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <random>
#include <thread>

//#include "gameDefine.h"

#define REDIS_POP_TIMEOUT    (1)

#define USE_REDIS_CLUSTER    (0)

#define ONE_DAY        60*60*24
#define ONE_WEEK        ONE_DAY*7

#define REDIS_QUARANTINE                "quarantine:"
#define REDIS_USER_BLOCK                "block:"
#define REDIS_BLACKLIST                 "blacklist:"
#define REDIS_FIELD_STATUS              "status"
#define REDIS_FIELD_CURRENT             "current"
#define REDIS_FIELD_TOTAL               "total"
#define REDIS_FIELD_RATE               "rate"
#define MAX_USER_ONLINE_INFO_IDLE_TIME   (60*3)
#if USE_REDIS_CLUSTER
    struct redisClusterContext;

#define REDIS_COMMAND    redisClusterCommand

#else
    struct redisContext;

#define REDIS_COMMAND    redisCommand

#endif

#ifndef CountArray
#define CountArray(a)   (sizeof(a)/sizeof(a[0]))
#endif//CountArray

namespace redis
{
	class RedisValItem
	{
	public:
		RedisValItem() {}
		virtual ~RedisValItem() {}

	public:
        int asInt()
        {
            if(val.empty())
                return 0;
            else
                return (stoi(val));
		}

        unsigned int asUInt()
        {
            if(val.empty())
                return 0;
            else
                return ((unsigned int)asInt());
		}

        long asLong()
        {
            if(val.empty())
                return 0;
            else
                return (stol(val));
		}

        int64_t asInt64()
        {
            if(val.empty())
                return 0;
            else
                return (stoll(val));
        }

        double asDouble()
        {
            if(val.empty())
                return 0.0;
            else
                return (stod(val));
		}

        // get the string value.
        std::string asString()
        {
            return val;
		}

	public:
		RedisValItem& operator=(std::string value)
		{
			val = value;
			return *this;
        }

		RedisValItem& operator=(long double value)
		{
			val = to_string(value);
			return *this;
		}

	protected:
		std::string val;
	};

	class RedisValue
	{
	public:
        RedisValue() {}
		virtual ~RedisValue() {}
		RedisValItem& operator[](std::string key)
		{
			std::map<std::string,RedisValItem>::iterator iter = listval.find(key);
            if (iter == listval.end())
            {
				listval[key]="";
			}

			RedisValItem& val = listval[key];
			return val;
		}

        void reset()
        {
            listval.clear();
        }

        bool empty()
        {
			return (listval.size()<=0);
		}

        // try to get the special map content now.
        std::map<std::string,redis::RedisValItem>& get()
        {
            return listval;
        }

	protected:
        std::map<std::string, RedisValItem> listval;
	};
}

// redis client.
class RedisClient
{

public:
    RedisClient();

    virtual ~RedisClient();

public:
    bool initRedisCluster(std::string ip, std::string password = "");
    bool initRedisCluster(std::string ip, std::map<std::string, std::string> &addrMap, std::string password = "");
    bool ReConnect();
    int  getMasterAddr(const std::vector<std::string> &addVec, struct timeval timeOut,std::string& masterIp, int& masterPort);

    bool get(std::string key, std::string &value);
    bool set(std::string key, std::string value, int timeout = 0);
    bool del(std::string key);
    int TTL(std::string key);
    bool exists(std::string key);
    bool persist(std::string key);

    bool hget(std::string key, std::string field, std::string &value);
    bool hset(std::string key, std::string field, std::string value, int timeout=0);

    bool hmget(std::string key, std::string* fields, int count, redis::RedisValue& redisValue);
    bool hmset(std::string key, redis::RedisValue& redisValue, int timeout=0);

	bool hmget(std::string key, std::string* fields, int count, STD::generic_map& m);
	bool hmset(std::string key, STD::generic_map& m, int timeout = 0);

    bool hmget(std::string key, std::vector<std::string>fields, std::vector<std::string> &values);
    bool hmset(std::string key, std::vector<std::string>fields, std::vector<std::string>values, int timeout=0);
    bool hmset(std::string key, std::map<std::string,std::string> fields,int timeout=0);
    bool hdel(std::string key, std::string field);
    bool exists(std::string key, std::string field);

    bool hincrby(std::string key, std::string field, int64_t inc, int64_t* result);
    bool hincrby_float(std::string key, std::string field, double inc, double* result);

    bool resetExpired(std::string key, int timeout = 60 * 3/*=MAX_USER_ONLINE_INFO_IDLE_TIME*/);
    bool resetExpiredEx(std::string key, int timeout=1000);
    
    // add by caiqing
    //推送公共消息
    void pushPublishMsg(int msgId,std::string msg);
    //订阅公共消息
    void subscribePublishMsg(int msgId,function<void(std::string)> func);
    // List 操作
//     bool lremCmd(eRedisKey keyId, int count, std::string value);
//     bool rpopCmd(eRedisKey keyId,std::string &lastElement);
//     bool lpushCmd(eRedisKey keyId,std::string value,long long &len);
//     bool lrangeCmd(eRedisKey keyId,std::vector<std::string> &list,int end,int start = 0); 
    // 集合操作
//     bool sremCmd(eRedisKey keyId,std::string value);  
//     bool saddCmd(eRedisKey keyId,std::string value); 
    //返回集合中的所有的成员（性能损耗大，适用于数量少数据）
//     bool smembersCmd(eRedisKey keyId,std::vector<std::string> &list); 
//     bool delnxCmd(eRedisKey keyId,std::string & lockValue);
//     int setnxCmd(eRedisKey keyId, std::string & value,int timeout);
private:
    bool lrem(std::string key, int count, std::string value);
    bool rpop(std::string key, std::string &values);
    bool sadd(std::string key, std::string value);        //向key的添加value值
    bool sismember(std::string key, std::string value);   //判断 member 元素是否是集合 key 的成员
    bool srem(std::string key, std::string value);        //删除key值中的value值
    bool smembers(std::string key, std::vector<std::string> &list);    //返回集合中的所有的成员
    //add end
private:
    bool blpop(std::string key, std::string &value, int timeOut);
    bool rpush(std::string key, std::string value);
    bool lpush(std::string key, std::string value, long long int &len);
    bool lrange(std::string key, int startIdx, int endIdx,std::vector<std::string> &values);
    bool ltrim(std::string key, int startIdx, int endIdx);
    bool llen(std::string key,int32_t &value);


public:
    bool ResetExpiredAccountUid(std::string const& account);
    bool GetAccountUid(std::string const& account, int64_t& userId);
    bool SetAccountUid(std::string const& account, int64_t userid);
    bool ResetExpiredToken(std::string const& token);
    bool SetTokenInfo(std::string const& token, int64_t userid, std::string const& account, std::string const& gateip = "");
    bool GetTokenInfo(std::string const& token,
        int64_t& userid, std::string& account, uint32_t& agentid);
    bool SetUserOnlineInfo(int64_t userId, uint32_t nGameId, uint32_t nRoomId);
    bool GetUserOnlineInfo(int64_t userId, uint32_t &nGameId, uint32_t &nRoomId);
    bool SetUserOnlineInfoIP(int64_t userId, std::string ip);
    bool GetUserOnlineInfoIP(int64_t userId, std::string &ip);
    bool ResetExpiredUserOnlineInfo(int64_t userId,int timeout = MAX_USER_ONLINE_INFO_IDLE_TIME);
    bool ExistsUserOnlineInfo(int64_t userId);
    bool DelUserOnlineInfo(int64_t userId);
    int TTLUserOnlineInfo(int64_t userId);
    bool GetGameServerplayerNum(std::vector<std::string> &serverValues,uint64_t &nTotalCount);
    bool GetGameRoomplayerNum(std::vector<std::string> &serverValues,std::map<std::string,uint64_t>& mapPlayerNum);
    bool GetGameAgentPlayerNum(std::vector<std::string> &keys,std::vector<std::string> &values);
public:
//    bool setUserLoginInfo(int64_t userId, Global_UserBaseInfo& userinfo);
//    bool GetUserLoginInfo(int64_t userId, Global_UserBaseInfo& userinfo);

    bool GetUserLoginInfo(int64_t userId, std::string field0, std::string &value0);
    bool SetUserLoginInfo(int64_t userId, std::string field, const std::string &value);

    bool ResetExpiredUserLoginInfo(int64_t userId);
    bool ExistsUserLoginInfo(int64_t userId);
    bool DeleteUserLoginInfo(int64_t userId);
//    int TTLUserLoginInfo(int64_t userId);
    bool AddToMatchedUser(int64_t userId, int64_t blockUser);
    bool GetMatchBlockList(int64_t userId,std::vector<std::string> &list);
    bool RemoveQuarantine(int64_t userId);
    bool AddQuarantine(int64_t userId);

public:

    //================Message Publish Subscribe============
    void publishRechargeScoreMessage(std::string msg);
    void subscribeRechargeScoreMessage(function<void(std::string)> func);

    void publishRechargeScoreToProxyMessage(std::string msg);
    void subscribeRechargeScoreToProxyMessage(function<void(std::string)> func);

    void publishRechargeScoreToGameServerMessage(std::string msg);
    void subscribeRechargeScoreToGameServerMessage(function<void(std::string)> func);

    void publishExchangeScoreMessage(std::string msg);
    void subscribeExchangeScoreMessage(function<void(std::string)> func);

    void publishExchangeScoreToProxyMessage(std::string msg);
    void subscribeExchangeScoreToProxyMessage(function<void(std::string)> func);

    void publishExchangeScoreToGameServerMessage(std::string msg);
    void subscribeExchangeScoreToGameServerMessage(function<void(std::string)> func);

    void publishUserLoginMessage(std::string msg);
    void subscribeUserLoginMessage(function<void(std::string)> func);

    void publishUserKillBossMessage(std::string msg);
    void subscribeUserKillBossMessage(function<void(std::string)> func);

    void publishNewChatMessage(std::string msg);
    void subscribeNewChatMessage(function<void(std::string)> func);

    void publishNewMailMessage(std::string msg);
    void subscribeNewMailMessage(function<void(std::string)> func);

    void publishNoticeMessage(std::string msg);
    void subscribeNoticeMessage(function<void(std::string)> func);

    void publishStopGameServerMessage(std::string msg);
    void subscribeStopGameServerMessage(function<void(std::string)> func);

    void publishRefreashConfigMessage(std::string msg);
    void subscribeRefreshConfigMessage(function<void(std::string)> func);

    void publishOrderScoreMessage(std::string msg);
    void subsreibeOrderScoreMessage(function<void(std::string)> func);

    void unsubscribe();
    void getSubMessage();
    void startSubThread();

private:
    bool auth(std::string pass);

    void publish(std::string channel, std::string msg);
    void subscribe(std::string channel);

public:
    bool PushSQL(std::string sql);
    bool POPSQL(std::string &sql, int timeOut);
    bool BlackListHget(std::string key, std::string keyson,redis::RedisValue& values,std::map<std::string,int16_t> &usermap);
private:
    shared_ptr<thread> m_redis_pub_sub_thread;
    std::map<std::string, function<void(std::string)> > m_sub_func_map;

private:
#if USE_REDIS_CLUSTER
    redisClusterContext *m_redisClientContext;
#else
    redisContext* m_redisClientContext;
#endif
    std::string        m_ip;
};




//    int setVerifyCode(std::string phoneNum, int type);  //0 getVerifycode ok   1 already set code 2 error
//    int getVerifyCode(std::string phoneNum, int type, std::string &verifyCode);  //0 getVerifycode ok   1 noet exists 2 error;
//    void setVerifyCode(std::string phoneNum, int type, std::string &verifyCode);
//    bool existsVerifyCode(std::string phoneNum, int type);

//    bool setUserLoginInfo(int64_t userId, std::string &account, std::string &password, std::string &dynamicPassword, int temp,
//                                     std::string &machineSerial, std::string &machineType, int nPlatformId, int nChannelId);

//    bool setUserIdGameServerInfo(int64_t userId, std::string ip);
//    bool getUserIdGameServerInfo(int64_t userId, std::string &ip);
//    bool resetExpiredUserIdGameServerInfo(int64_t userId);
//    bool existsUserIdGameServerInfo(int64_t userId);
//    bool delUserIdGameServerInfo(int64_t userId);
//    bool persistUserIdGameServerInfo(int64_t userId);
//    int TTLUserIdGameServerInfo(int64_t userId);




#endif // REDISCLIENT_H
