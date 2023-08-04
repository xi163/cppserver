#include "public/Inc.h"
#include "RedisClient.h"
#include "../redisKeys.h"
#include <thread>


#ifdef __cplusplus
extern "C" {
#endif

#include "hiredis.h"
#include "hircluster.h"

#ifdef __cplusplus
}
#endif

//#include "json/json.h"

#define REDIS_ACCOUNT_PREFIX        "h.uid."
#define REDIS_ONLINE_PREFIX         "h.online.uid.gameinfo."
//#define prefix_account_uid         "k.account.uid."
//#define prefix_token               "k.token."
#define MAX_USER_ONLINE_INFO_IDLE_TIME   (60*3)
//#include <muduo/base/Logging.h>
//#include <boost/algorithm/std::string.hpp>
//#include <algorithm>
//#include "../crypto/crypto.h"

const std::string passWord = "AliceLandy@20181024";


std::string string_replace(std::string strbase, std::string src, std::string dst)
{
    std::string::size_type pos = 0;
    std::string::size_type srclen = src.size();
    std::string::size_type dstlen = dst.size();
    pos = strbase.find(src,pos);
    while (pos != std::string::npos)
    {
        strbase.replace(pos, srclen, dst);
        pos = strbase.find(src,(pos+dstlen));
    }

    return strbase;
}

RedisClient::RedisClient()
{
    m_redisClientContext = NULL;
}

RedisClient::~RedisClient()
{
#if USE_REDIS_CLUSTER
    if(m_redisClientContext)
        redisClusterFree(m_redisClientContext);
#else
    if(m_redisClientContext)
        redisFree(m_redisClientContext);
#endif
}

#if USE_REDIS_CLUSTER
bool RedisClient::initRedisCluster(std::string ip)
{
    m_redisClientContext = redisClusterContextInit();
    redisClusterSetOptionAddNodes(m_redisClientContext, ip.c_str());
    redisClusterConnect2(m_redisClientContext);
    if (m_redisClientContext != NULL && m_redisClientContext->err)
    {
        _LOG_ERROR("Error: %s\n\n\n\n", m_redisClientContext->errstr);
        // handle error
        return false;
    }else
        return true;
}

#elif 0

bool RedisClient::initRedisCluster(std::string ip)
{
//    struct timeval timeout = {1, 500000 };
//    m_redisClientContext = redisConnectWithTimeout(ip.c_str(), 6379, timeout);
    m_redisClientContext = redisConnect(ip.c_str(), 6379);

    if(m_redisClientContext->err)
        _LOG_ERROR("redis %s\n\n\n\n", m_redisClientContext->errstr);
    return !m_redisClientContext->err;
}

#else
bool RedisClient::BlackListHget(std::string key,  std::string keyson, redis::RedisValue& values,std::map<std::string,int16_t> &usermap)
{
    bool OK = false;
    values.reset();
    usermap.clear();
    while(true)
    {
        std::string cmd = "HMGET " + key+" "+keyson;

        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;

            std::vector<std::string> vec;
            if(!reply->element||!reply->element[0]->str)
            {
                OK = false;
                break;
            }
            std::string covers=reply->element[0]->str;
            covers.erase(std::remove(covers.begin(), covers.end(), '"'), covers.end());
            boost::algorithm::split( vec,covers, boost::is_any_of( "|" ));
            for(int i=0;i<(int)vec.size();i++)
            {
                std::vector<std::string> vec1;
                boost::algorithm::split( vec1,vec[i] , boost::is_any_of( "," ));
                if(vec1.size()==2)
                {
                    usermap.insert(pair<std::string,int16_t>(vec1[0],stoi(vec1[1])));
                }
                vec1.clear();
            }

            freeReplyObject(reply);
            break;
        }else
            break;
    }


    return OK;
}

bool RedisClient::initRedisCluster(std::string ip, std::string password)
{
    std::string masterIp("127.0.0.1");
    int masterPort=6379;

    struct timeval timev;
    timev.tv_sec = 3;
    timev.tv_usec = 0;

    std::vector<std::string> vec;
    boost::algorithm::split(vec, ip, boost::is_any_of( "," ));
    if(getMasterAddr(vec, timev, masterIp, masterPort) == 0)
    {

    }else
    {
        std::vector<std::string> ipportVec;
        boost::algorithm::split(ipportVec, ip, boost::is_any_of( ":" ));
        masterIp = ipportVec[0];
        masterPort = stoi(ipportVec[1]);
    }

    m_redisClientContext = redisConnectWithTimeout(masterIp.c_str(), masterPort, timev);
    if(m_redisClientContext->err)
    {
        _LOG_ERROR("redis %s\n\n\n\n", m_redisClientContext->errstr);
        return false;
    }else
    {
        m_ip = ip;
    }
    if(!password.empty())
        return auth(password);
    else
        return true;
}

bool RedisClient::initRedisCluster(std::string ip, std::map<std::string, std::string> &addrMap, std::string password)
{
    std::string masterIp("127.0.0.1");
    int masterPort=6379;

    struct timeval timev;
    timev.tv_sec = 3;
    timev.tv_usec = 0;

    std::vector<std::string> vec;
    boost::algorithm::split(vec, ip, boost::is_any_of( "," ));
    if(getMasterAddr(vec, timev, masterIp, masterPort) == 0)
    {
        if(addrMap.count(masterIp))
            masterIp = addrMap[masterIp];
    }else
    {
        std::vector<std::string> ipportVec;
        boost::algorithm::split(ipportVec, ip, boost::is_any_of( ":" ));
        masterIp = ipportVec[0];
        masterPort = stoi(ipportVec[1]);
    }

    m_redisClientContext = redisConnectWithTimeout(masterIp.c_str(), masterPort, timev);
    if(m_redisClientContext->err)
    {
        _LOG_ERROR("redis %s\n\n\n\n", m_redisClientContext->errstr);
        return false;
    }else
    {
        m_ip = ip;
    }
    if(!password.empty())
        return auth(password);
    else
        return true;
}

#endif

//通过Sentinel获得master的ip和port
int RedisClient::getMasterAddr(const std::vector<std::string> &addVec, struct timeval timeOut,std::string& masterIp, int& masterPort)
{
    //建立和sentinel的连接
    std::string strPort;
    redisContext* context = NULL;
    redisReply *reply = NULL;
    for(int i = 0; i < addVec.size(); ++i)
    {
        std::vector<std::string> vec;
        boost::algorithm::split(vec, addVec[i], boost::is_any_of( ":" ));

        //_LOG_INFO("i[%d], ip[%s], port[%d]", i, vec[0].c_str(), stoi(vec[1]));
        context = redisConnectWithTimeout(vec[0].c_str(), stoi(vec[1]), timeOut);
//        context = redisConnect(vec[0].c_str(), stoi(vec[1]));
        if (context == NULL || context->err)
        {
            _LOG_ERROR("Connection error: can't allocate redis context,will find next");
            redisFree(context);//断开连接并释放redisContext空间
            continue;
        }

        //获取master的ip和port
        reply = static_cast<redisReply*> ( redisCommand(context,"SENTINEL get-master-addr-by-name mymaster") );
        if(reply->type != REDIS_REPLY_ARRAY || reply -> elements != 2)
        {
            //_LOG_ERROR("use sentinel to get-master-addr-by-name failure, will find next");
            freeReplyObject(reply);
            continue;
        }
        masterIp = reply -> element[0] -> str;
        strPort = reply -> element[1] -> str;
        masterPort = stoi(strPort);
        break;
    }
    if(masterIp.empty() || strPort.empty())
        return -1;
    return 0;
}

bool RedisClient::auth(std::string pass)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "AUTH %s", pass.c_str());
        if(reply)
        {
            if(reply->str  && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::ReConnect()
{
//    return REDIS_OK==redisReconnect(m_redisClientContext);
    return initRedisCluster(m_ip);
}

bool RedisClient::get(std::string key, std::string &value)
{
    bool OK = false;
    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "GET %s", key.c_str());
        if(reply)
        {
            if(reply->str)
            {
                value = reply->str;
                OK = true;
            }else
                value = "";
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::set(std::string key, std::string value, int timeout)
{
    bool OK = false;

    while(true)
    {
        redisReply *reply = NULL;
        if(timeout)
        {
            reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SET %s %s EX %d", key.c_str(), value.c_str(), timeout);
        }else
        {
            reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SET %s %s", key.c_str(), value.c_str());
        }

        if(reply)
        {
//            LOG_INFO << " >>> redisClient::set reply type:" << reply->type;
//            LOG_INFO << " >>> std::string value:" << reply->str << ",integer:" << reply->integer << "timeout:" << timeout;

            if(reply->str  && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    return OK;
}

bool RedisClient::del(std::string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "DEL %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

int RedisClient::TTL(std::string key)
{
    int ret = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "TTL %s", key.c_str());
        if(reply)
        {
            ret = reply->integer;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return ret;
}

bool RedisClient::resetExpired(std::string key, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "EXPIRE %s %d", key.c_str(), timeout);
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
// 过期时间为毫秒
bool RedisClient::resetExpiredEx(std::string key, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PEXPIRE %s %d", key.c_str(), timeout);
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::exists(std::string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "EXISTS %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::persist(std::string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PERSIST %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }


//  LOG_DEBUG << " >>> RedisClient::persist key:" << key << ", Ok:" << OK;
    return OK;
}

bool RedisClient::hget(std::string key, std::string field, std::string &value)
{
    bool OK = false;
    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HGET %s %s", key.c_str(), field.c_str());
        if(reply)
        {
    //      LOG_INFO << " >>> redisClient::get key:" << key << ",field:" << field << ", reply type:" << reply->type;
    //      LOG_INFO << " >>> std::string value:" << reply->str << ",integer:" << reply->integer;

            if(reply->str)
            {
                value = reply->str;
                OK = true;
            }else
                value = "";
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hset(std::string key, std::string field, std::string value, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
        if(reply)
        {
            if(reply->str && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);

            // set expired.
            if(timeout)
            {
                resetExpired(key, timeout);
            }
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hmget(std::string key, std::string* fields, int count, redis::RedisValue& values)
{
    bool OK = false;
    values.reset();

    while(true)
    {
        if ((!fields)||(!count))
            break;
        std::string cmd = "HMGET " + key;
        for (int i=0;i<count;i++)
        {
            cmd += " " + fields[i];
        }

        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                redisReply *tmpReply = reply->element[i];
                std::string field = fields[i];
                if(tmpReply->str)
                {
                    values[field] = tmpReply->str;
                }
//                else {
//                    values[field] = "";
//                }
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }


    return OK;
}

bool RedisClient::hmset(std::string key, redis::RedisValue& values, int timeout)
{
    bool OK = false;
    std::string cmd = "HMSET " + key;
    std::map<std::string,redis::RedisValItem> listval = values.get();
    std::map<std::string,redis::RedisValItem>::iterator iter;
    for (iter=listval.begin();iter!=listval.end();++iter)
    {
        std::string field = iter->first;
        redis::RedisValItem& item = iter->second;
        std::string value = item.asString();
        if (value.length())
        {
            value = string_replace(value," ","_");
            cmd += " " + field + " " + value;
        }
    }

    while(true)
    {
        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = false;
            if(reply->str && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    // set expired.
    if (timeout)
    {
        resetExpired(key,timeout);
    }   else
    {
        persist(key);
    }

    return OK;
}

bool RedisClient::hmget(std::string key, std::vector<std::string>fields, std::vector<std::string> &values)
{
    bool OK = false;
    values.resize(fields.size());

    std::string cmd = "HMGET " + key;
    for(std::string field : fields)
    {
        cmd += " "+field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                redisReply *tmpReply = reply->element[i];
                if(tmpReply->str)
                    values[i] = tmpReply->str;
                else
                    values[i] = "";
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hmset(std::string key, std::vector<std::string>fields, std::vector<std::string> values,int timeout)
{
    bool OK = false;
    std::string cmd = "HMSET " + key;
    if(fields.size() == values.size())
    {
        for(int i = 0; i < fields.size(); ++i)
        {
            if (values[i].length())
            {
                cmd += " "+fields[i] + " "+values[i];
            }
        }

        while(true)
        {
            redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
            if(reply)
            {
                if(reply->str && std::string(reply->str) == "OK")
                    OK = true;
                freeReplyObject(reply);
                break;
            }else
                ReConnect();
        }
    }

    // set expired.
    if (timeout)
    {
        resetExpired(key,timeout);
    }   else
    {
        persist(key);
    }

    return OK;
}

bool RedisClient::hmset(std::string key, std::map<std::string,std::string> fields,int timeout)
{
    bool OK = false;
    std::string cmd = "HMSET " + key;
    std::map<std::string,std::string>::iterator iter;
    for (iter=fields.begin();iter!=fields.end();++iter)
    {
        std::string field = iter->first;
        std::string value = iter->second;
        if (value.length())
        {
            cmd += " " + field + " " + value;
        }
    }

    while(true)
    {
        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            if(reply->str && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    // set expired.
    if (timeout) {
        resetExpired(key,timeout);
    }else
    {
        persist(key);
    }

    return OK;

}

bool RedisClient::hdel(std::string key, std::string field)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HDEL %s %s", key.c_str(), field.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::exists(std::string key, std::string field)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HEXISTS %s %s", key.c_str(), field.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
// edit by caiqing
// @long -> int64_t
bool RedisClient::hincrby(std::string key, std::string field, int64_t inc, int64_t* result)
{
    bool ok = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HINCRBY %s %s %lld", key.c_str(), field.c_str(), inc);
        if(reply)
        {
            if (reply->type == REDIS_REPLY_INTEGER)
            {
                if (result)
                    *result = reply->integer;
                ok = true;
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }


    return ok;
}

bool RedisClient::hincrby_float(std::string key, std::string field, double inc, double* result)
{
    std::string tmp("");
    bool ok = false;
    std::string strIncNum = std::to_string(inc);
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HINCRBYFLOAT %s %s %s", key.c_str(), field.c_str(), strIncNum.c_str());
        if(reply)
        {
            if(reply->type == REDIS_REPLY_STRING)
            {
                tmp = reply->str;
                if (result)
                    *result = strtod(tmp.c_str(),NULL);
                ok = true;
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }


    return ok;
}

bool RedisClient::blpop(std::string key, std::string &value, int timeOut)
{
    bool OK = false;

    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "BLPOP %s %d", key.c_str(), timeOut);
        if(reply)
        {
            if(reply->elements == 2)
            {
                redisReply *tmpKey = reply->element[0];
                if(tmpKey && tmpKey->str)
                {
                    if(!strcmp(tmpKey->str, key.c_str()))
                    {
                        redisReply *tmpValue = reply->element[1];
                        if(tmpValue && tmpValue->str)
                        {
                            value = tmpValue->str;
                            OK = true;
                        }

                    }
                }
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    return OK;
}

// LLen key : query queue size.
bool RedisClient::rpush(std::string key, std::string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "RPUSH %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// LLen key : query queue size.
bool RedisClient::lpush(std::string key, std::string value, long long int &len)
{
    bool OK = false;
    len = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LPUSH %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            if(reply->integer > 0){
                OK = true;
                len = reply->integer;
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// 移出并获取列表的第一个元素
bool RedisClient::rpop(std::string key, std::string &values){
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "RPOP %s", key.c_str());
        if(reply)
        {
            OK = true; 
            if(reply->str){
                values = std::string(reply->str);
                // LOG_ERROR << __FILE__ << " " << values << " " << __FUNCTION__;
                // printf("rpop :%s\n",reply->str);
            } 
            
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::lrange(std::string key, int startIdx, int endIdx,std::vector<std::string> &values)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LRANGE %s %d %d", key.c_str(), startIdx, endIdx);
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    if(values.size() > i)
                    {
                        values[i] = reply->element[i]->str;
                    }else{
                        values.push_back(reply->element[i]->str);
                    }

                }else{
                    break;
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::ltrim(std::string key, int startIdx, int endIdx)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LTRIM %s %d %d", key.c_str(), startIdx, endIdx);
        if(reply)
        {
            if(reply->str  && std::string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::llen(std::string key,int32_t &value)
{
    bool OK = false;
    value = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LLEN %s", key.c_str());
        if(reply)
        {
            if(reply->str)
            {
                value = reply->integer;
                OK = true;
            }else
                value = 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// add by caiqing 
// 列表移除元素
bool RedisClient::lrem(std::string key, int count, std::string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LREM %s %d %s", key.c_str(), count, value.c_str());
        if(reply)
        {
            OK = reply->integer > 0; 
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
 
// 移除列表元素
// count = 0 : 移除与 VALUE 相等的元素，数量为 COUNT 
// count > 0 : 从表头开始向表尾搜索
// count < 0 : 从表尾开始向表头搜索
// bool RedisClient::lremCmd(eRedisKey keyId, int count, std::string value){
//     std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//     return lrem(keyName,count,value);
// }

// 列表添加元素
// bool RedisClient::lpushCmd(eRedisKey keyId,std::string value,long long &len){
//     std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//     return lpush(keyName,value,len);
// }

// 获取列表元素
// bool RedisClient::lrangeCmd(eRedisKey keyId,std::vector<std::string> &list,int end,int start){
//    std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//    bool result = lrange(keyName,start,end,list);
//    return result;
// }

// 移除列表元素
// bool RedisClient::rpopCmd(eRedisKey keyId,std::string &lastElement){
//    std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//    bool result = rpop(keyName,lastElement);
//    return result;
// }

// 移除集合中一个或多个成员
// bool RedisClient::sremCmd(eRedisKey keyId,std::string value){
//     std::string keyName = REDIS_KEY_ID + to_string((int)keyId);
//     return  srem(keyName, value);
// } 

//为集合添加元素
// bool RedisClient::saddCmd(eRedisKey keyId,std::string value){
//     std::string keyName = REDIS_KEY_ID + to_string((int)keyId); 
//     return sadd(keyName, value);
// }

// 获取集合元素
// bool RedisClient::smembersCmd(eRedisKey keyId,std::vector<std::string> &list){
//    std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//    bool result = smembers(keyName,list);
//    return result;
// }
// 移除锁
// bool RedisClient::delnxCmd(eRedisKey keyId,std::string & lockValue){
//     std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//     std::string values;
//     get(keyName,values);
//     // 相同才能解锁
//     if(values == lockValue){
//         return del(keyName);
//     }
//     else
//         return false;
// }
// 设置锁(使用条件:允许锁的偶尔失效)
// int RedisClient::setnxCmd(eRedisKey keyId, std::string &value,int timeout){
//     std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//     int ret = -1,repeat = 0;
//     while( ++repeat < 1000 ){
//         redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SETNX %s %s", keyName.c_str(), value.c_str());
//         if(reply){
//             ret = reply->integer;  
//             freeReplyObject(reply);
//             break;
//         }else
//             ReConnect();
//     }
// 
//     // 设置过期时间
//     if (ret == 1 && timeout > 0)
//         resetExpired(keyName, timeout);
// 
//     return ret;
// }
 
//创建集合
bool RedisClient::sadd(std::string key, std::string value)
{
    // LOG_ERROR << __FILE__ << " " << key << " " << value << " " << __FUNCTION__;
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SADD %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0; 
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

//判断Key是否为集合
bool RedisClient::sismember(std::string key, std::string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SISMEMBER %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::srem(std::string key, std::string value)
{
    // LOG_ERROR << __FILE__ << " " << key << " " << value << " " << __FUNCTION__;

    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SREM %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::smembers(std::string key, std::vector<std::string> &list)
{
    bool OK = false;
    list.clear();
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SMEMBERS %s", key.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                // LOG_WARN << "elements = "<< i << " "<< reply->element[i]->str;
                // 查找完
                if(strcmp(reply->element[i]->str,"nil") == 0){
                    // LOG_ERROR << "smembers 查找完 = ";
                    break;
                }

                if(reply->element[i]->str){
                    list.push_back(reply->element[i]->str);
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::ExpireAccountUid(std::string const& account) {
    std::string key = redisKeys::prefix_account_uid + account;
    return resetExpired(key, redisKeys::Expire_AccountUid);
}

bool RedisClient::GetAccountUid(std::string const& account, int64_t& userId) {
    std::string key = redisKeys::prefix_account_uid + account;
    std::string val;
    bool bv = get(key, val);
    if (!val.empty()) {
        userId = stoll(val);
    }
    else {
        userId = 0;
    }
    return bv;
}

bool RedisClient::SetAccountUid(std::string const& account, int64_t userid) {
    std::string key = redisKeys::prefix_account_uid + account;
    return set(key, std::to_string(userid), redisKeys::Expire_AccountUid);
}

bool RedisClient::SetToken(std::string const& token, int64_t userid, std::string const& account) {
    std::string key = redisKeys::prefix_token + token;
    BOOST::Json obj;
    obj.put("account", account);
    obj.put("uid", userid);
    return set(key, obj.to_json(), redisKeys::Expire_Token);
}

//=================================================
bool RedisClient::SetUserOnlineInfo(int64_t userId, uint32_t nGameId, uint32_t nRoomId)
{
   std::string strKeyName = REDIS_ONLINE_PREFIX+ to_string(userId);

    redis::RedisValue values;
    values["nGameId"] = nGameId;
    values["nRoomId"] = nRoomId;
    return hmset(strKeyName, values, MAX_USER_ONLINE_INFO_IDLE_TIME);
}

bool RedisClient::GetUserOnlineInfo(int64_t userId, uint32_t &nGameId, uint32_t &nRoomId)
{
    bool ok = false;
    redis::RedisValue redisValue;
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    std::string fields[]   = {"nGameId", "nRoomId"};
    bool bExist = hmget(strKeyName, fields, CountArray(fields), redisValue);
    if  ((bExist) && (!redisValue.empty()))
    {
        nGameId     = redisValue["nGameId"].asInt();
        nRoomId     = redisValue["nRoomId"].asInt();
        ok = true;
    }
    return ok;
}

bool RedisClient::SetUserOnlineInfoIP(int64_t userId, std::string ip)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return hset(strKeyName, "GameServerIP", ip, MAX_USER_ONLINE_INFO_IDLE_TIME);
}

bool RedisClient::GetUserOnlineInfoIP(int64_t userId, std::string &ip)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return hget(strKeyName, "GameServerIP", ip);
}

bool RedisClient::ResetExpiredUserOnlineInfo(int64_t userId,int timeout)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return resetExpired(strKeyName,timeout);
}

bool RedisClient::ExistsUserOnlineInfo(int64_t userId)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return exists(strKeyName);
}

bool RedisClient::DelUserOnlineInfo(int64_t userId)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return del(strKeyName);
}

int RedisClient::TTLUserOnlineInfo(int64_t userId)
{
    std::string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
    return TTL(strKeyName);
}

bool RedisClient::GetGameServerplayerNum(std::vector<std::string> &serverValues,uint64_t &nTotalCount)
{
    bool OK = false;
    std::string cmd = "MGET " ;
    for(std::string &field : serverValues)
    {
        cmd += " " + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    std::vector<std::string> vec{"0","0"};
                    boost::algorithm::split(vec, reply->element[i]->str, boost::is_any_of( "+" ));
                    nTotalCount+=(stod(vec[0])+stod(vec[1]));
//                      nTotalCount+=(stod(reply->element[i]->str));
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::GetGameRoomplayerNum(std::vector<std::string> &serverValues, std::map<std::string, uint64_t> &mapPlayerNum)
{
    bool OK = false;
    std::string cmd = "MGET " ;
    for(std::string &field : serverValues)
    {
        cmd += " " + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    std::vector<std::string> vec{"0","0"};
                    boost::algorithm::split(vec, reply->element[i]->str, boost::is_any_of( "+" ));
                    mapPlayerNum.emplace(serverValues[i],stod(vec[0]));
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::GetGameAgentPlayerNum(std::vector<std::string> &keys, std::vector<std::string> &values)
{
    bool OK = false;
    std::string cmd = "MGET " ;
    for(std::string &field : keys)
    {
        cmd += " agentid:" + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    values.push_back(reply->element[i]->str);
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::GetUserLoginInfo(int64_t userId, std::string field, std::string &value)
{
    bool ret = false;
    value = "";
    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
    if (hget(key, field, value))
    {
        ret = true;
    }
    return ret;
}

bool RedisClient::SetUserLoginInfo(int64_t userId, std::string field, const std::string &value)
{
    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
    return hset(key, field, value);
}

bool RedisClient::ResetExpiredUserLoginInfo(int64_t userId)
{
    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
    return resetExpired(key);
}

bool RedisClient::ExistsUserLoginInfo(int64_t userId)
{
    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
    return exists(key);
}

bool RedisClient::DeleteUserLoginInfo(int64_t userId)
{
    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
    return del(key);
}

bool RedisClient::AddToMatchedUser(int64_t userId, int64_t blockUser)
{
    std::string key = REDIS_USER_BLOCK+to_string(userId);
    long long int newLen = 0;
    bool result = lpush(key,to_string(blockUser),newLen);
    if(result)
    {
        if(newLen > 20)
        {
            result = ltrim(key,0,19);
        }
    }
    resetExpired(key,ONE_WEEK);
    if(blockUser != 0 )
    {
        key = REDIS_USER_BLOCK+to_string(blockUser);
        result = lpush(key,to_string(userId),newLen);
        if(result)
        {
            if(newLen > 20)
            {
                result = ltrim(key,0,19);
            }
        }
        resetExpired(key,ONE_WEEK);
    }
    return true;
}

bool RedisClient::GetMatchBlockList(int64_t userId,std::vector<std::string> &list)
{
    std::string key = REDIS_USER_BLOCK+to_string(userId);
    bool result = lrange(key,0,19,list);
    return result;
}

bool RedisClient::AddQuarantine(int64_t userId)
{
    std::string key = REDIS_QUARANTINE+to_string(userId);
    std::string value;
    bool res = get(key,value);
    if(res)
    {
        if(value != "2")
        {
            set(key,"1");
            resetExpired(key, ONE_WEEK);
        }
    }else
    {
        set(key,"1");
        resetExpired(key, ONE_WEEK);
    }
}

bool RedisClient::RemoveQuarantine(int64_t userId)
{
    std::string key = REDIS_QUARANTINE+to_string(userId);
    std::string value;
    bool res = get(key,value);
    if(res)
    {
        if(value != "2")
        {
            del(key);
        }
    }
}

//int RedisClient::TTLUserLoginInfo(int64_t userId)
//{
//    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
//    return TTL(key);
//}









//=================================================
//发布
void RedisClient::publish(std::string channel, std::string msg)
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PUBLISH %s %s", channel.c_str(), msg.c_str());
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
}

//订阅
void RedisClient::subscribe(std::string channel)
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SUBSCRIBE %s", channel.c_str());
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
}

void RedisClient::unsubscribe()
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "UNSUBSCRIBE");
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    m_sub_func_map.clear();
}


void RedisClient::publishRechargeScoreMessage(std::string msg)
{
    publish("RechargeScoreMessage", msg);
}
void RedisClient::subscribeRechargeScoreMessage(function<void(std::string)> func)
{
    subscribe("RechargeScoreMessage");
    m_sub_func_map["RechargeScoreMessage"] = func;
}

void RedisClient::publishRechargeScoreToProxyMessage(std::string msg)
{
    publish("RechargeScoreToProxyMessage", msg);
}
void RedisClient::subscribeRechargeScoreToProxyMessage(function<void(std::string)> func)
{
    subscribe("RechargeScoreToProxyMessage");
    m_sub_func_map["RechargeScoreToProxyMessage"] = func;
}

void RedisClient::publishRechargeScoreToGameServerMessage(std::string msg)
{
    publish("RechargeScoreToGameServerMessage", msg);
}
void RedisClient::subscribeRechargeScoreToGameServerMessage(function<void(std::string)> func)
{
    subscribe("RechargeScoreToGameServerMessage");
    m_sub_func_map["RechargeScoreToGameServerMessage"] = func;
}

void RedisClient::publishExchangeScoreMessage(std::string msg)
{
    publish("ExchangeScoreMessage", msg);
}
void RedisClient::subscribeExchangeScoreMessage(function<void(std::string)> func)
{
    subscribe("ExchangeScoreMessage");
    m_sub_func_map["ExchangeScoreMessage"] = func;
}

void RedisClient::publishExchangeScoreToProxyMessage(std::string msg)
{
    publish("ExchangeScoreToProxyMessage", msg);
}
void RedisClient::subscribeExchangeScoreToProxyMessage(function<void(std::string)> func)
{
    subscribe("ExchangeScoreToProxyMessage");
    m_sub_func_map["ExchangeScoreToProxyMessage"] = func;
}

void RedisClient::publishExchangeScoreToGameServerMessage(std::string msg)
{
    publish("ExchangeScoreToGameServerMessage", msg);
}
void RedisClient::subscribeExchangeScoreToGameServerMessage(function<void(std::string)> func)
{
    subscribe("ExchangeScoreToGameServerMessage");
    m_sub_func_map["ExchangeScoreToGameServerMessage"] = func;
}


//推送公共消息
void RedisClient::pushPublishMsg(int msgId,std::string msg)
{
     publish("rs_public_msg_" + std::to_string((int)msgId), msg);
}
//订阅公共消息
void RedisClient::subscribePublishMsg(int msgId,function<void(std::string)> func)
{
    std::string msgName = "rs_public_msg_" + std::to_string((int)msgId);
    subscribe(msgName);
    m_sub_func_map[msgName] = func;
}

void RedisClient::publishUserLoginMessage(std::string msg)
{
    publish("UserLoginMessage", msg);
}

void RedisClient::subscribeUserLoginMessage(function<void(std::string)> func)
{
    subscribe("UserLoginMessage");
    m_sub_func_map["UserLoginMessage"] = func;
}

void RedisClient::publishUserKillBossMessage(std::string msg)
{
    publish("UserKillBossMessage", msg);
}
void RedisClient::subscribeUserKillBossMessage(function<void(std::string)> func)
{
    subscribe("UserKillBossMessage");
    m_sub_func_map["UserKillBossMessage"] = func;
}

void RedisClient::publishNewChatMessage(std::string msg)
{
    publish("NewChatMessage", msg);
}
void RedisClient::subscribeNewChatMessage(function<void(std::string)> func)
{
    subscribe("NewChatMessage");
    m_sub_func_map["NewChatMessage"] = func;
}

void RedisClient::publishNewMailMessage(std::string msg)
{
    publish("NewMailMessage", msg);
}
void RedisClient::subscribeNewMailMessage(function<void(std::string)> func)
{
    subscribe("NewMailMessage");
    m_sub_func_map["NewMailMessage"] = func;
}

void RedisClient::publishNoticeMessage(std::string msg)
{
    publish("NoticeMessage", msg);
}
void RedisClient::subscribeNoticeMessage(function<void(std::string)> func)
{
    subscribe("NoticeMessage");
    m_sub_func_map["NoticeMessage"] = func;
}

void RedisClient::publishStopGameServerMessage(std::string msg)
{
    publish("StopGameServerMessage", msg);
}
void RedisClient::subscribeStopGameServerMessage(function<void(std::string)> func)
{
    subscribe("StopGameServerMessage");
    m_sub_func_map["StopGameServerMessage"] = func;
}

void RedisClient::publishRefreashConfigMessage(std::string msg)
{
    publish("RefreashConfigMessage", msg);
}

void RedisClient::subscribeRefreshConfigMessage(function<void (std::string)> func)
{
    subscribe("RefreashConfigMessage");
    m_sub_func_map["RefreashConfigMessage"] = func;
}

void RedisClient::publishOrderScoreMessage(std::string msg)
{
    publish("OrderScoreMessage",msg);
}

void RedisClient::subsreibeOrderScoreMessage(function<void (std::string)> func)
{
    subscribe("OrderScoreMessage");
    m_sub_func_map["OrderScoreMessage"] = func;
}


void RedisClient::startSubThread()
{
    m_redis_pub_sub_thread.reset(new std::thread(&RedisClient::getSubMessage, this));
}

void RedisClient::getSubMessage()
{
    while(true)
    {
        redisReply *preply = NULL;

#if USE_REDIS_CLUSTER
        redisClusterGetReply(m_redisClientContext, (void **)&preply);
#else
        redisGetReply(m_redisClientContext, (void **)&preply);
#endif
        if(preply)
        {
            std::string msgtype, channel, msg;
            if(preply->type == REDIS_REPLY_ARRAY)
            {
                if(preply->elements >= 3)
                {
                    // msg type
                    msgtype = preply->element[0]->str;
                    if(msgtype == "message")
                    {
                        // ID
                        channel = preply->element[1]->str;
                        // Msg Body
                        msg = preply->element[2]->str;
                        if(m_sub_func_map.count(channel))
                        {
                            function<void(std::string)> functor = m_sub_func_map[channel];
                            functor(msg);
                        }
                    }
                }
            }
        }else
            ReConnect();
        freeReplyObject(preply);
    }
}

bool RedisClient::PushSQL(std::string sql)
{
    return rpush("SQLQueue", sql);
}

bool RedisClient::POPSQL(std::string &sql, int timeOut)
{
    return blpop("SQLQueue", sql, timeOut);
}





////request on game server
//bool RedisClient::setUserIdGameServerInfo(int userId, std::string ip)
//{
//    return set("GameServer:"+to_string(userId), ip, MAX_LOGIN_IDLE_TIME);
//}

//bool RedisClient::getUserIdGameServerInfo(int userId, std::string &ip)
//{
//    ip = "";
//    return get("GameServer:" + to_string(userId), ip);
//}

//bool RedisClient::resetExpiredUserIdGameServerInfo(int userId)
//{
//    return resetExpired("GameServer:" + to_string(userId));
//}

//bool RedisClient::existsUserIdGameServerInfo(int userId)
//{
//    return exists("GameServer:" + to_string(userId));
//}

//bool RedisClient::delUserIdGameServerInfo(int userId)
//{
//    return del("GameServer:"+to_string(userId));
//}

//bool RedisClient::persistUserIdGameServerInfo(int userId)
//{
//    return persist("GameServer:"+to_string(userId));
//}

//int RedisClient::TTLUserIdGameServerInfo(int userId)
//{
//    std::string key = "GameServer:" + to_string(userId);
//    return TTL(key);
//}

//int RedisClient::getVerifyCode(std::string phoneNum, int type, std::string &verifyCode)  //0 getVerifycode ok   1 noet exists 2 error
//{
//    std::string key = phoneNum + "_" + to_string(type);
//    if(get(key, verifyCode) && !verifyCode.empty())
//        return 0;
//    else
//        return 1;
//}

//void RedisClient::setVerifyCode(std::string phoneNum, int type, std::string &verifyCode)
//{
//    std::string key = phoneNum + "_" + to_string(type);
//    set(key, verifyCode, MAX_VERIFY_CODE_LOGIN_IDLE_TIME);
//}

//bool RedisClient::existsVerifyCode(std::string phoneNum, int type)
//{
//    std::string key = phoneNum + "_" + to_string(type);
//    return exists(key);
//}

//bool RedisClient::setUserLoginInfo(int userId, std::string &account, std::string &password, std::string &dynamicPassword, int temp,
//                                 std::string &machineSerial, std::string &machineType, int nPlatformId, int nChannelId)
//{


//    std::string key = "LoginInfo_" + to_string(userId);
//    Json::Value jsonValue;
//    jsonValue["userId"] = userId;
//    jsonValue["strAccount"] = account;
//    jsonValue["strPassword"] = password;
//    jsonValue["dynamicPassword"] = dynamicPassword;
//    jsonValue["temp"] = temp;
//    jsonValue["strMachineSerial"] = machineSerial;
//    jsonValue["strMachineType"] = machineType;
//    jsonValue["nPlatformId"] = nPlatformId;
//    jsonValue["nChannelId"] = nChannelId;
//    Json::FastWriter writer;
//    std::string strValue = writer.write(jsonValue);
//    return set(key, strValue);
//}

//// cache the special user login info.
//bool RedisClient::setUserLoginInfo(int64_t userId, Global_UserBaseInfo& baseinfo)
//{
//    redis::RedisValue redisValue;
//    std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);

//    redisValue["userId"]             = to_string((int)baseinfo.nUserId);
//    redisValue["proxyId"]            = to_string((int)baseinfo.nPromoterId);
//    redisValue["bindProxyId"]        = to_string((int)baseinfo.nBindPromoterId);

//    redisValue["gem"]                = to_string((int)baseinfo.nGem);
//    redisValue["platformId"]         = to_string((int)baseinfo.nPlatformId);
//    redisValue["channelId"]          = to_string((int)baseinfo.nChannelId);
//    redisValue["ostype"]             = to_string((int)baseinfo.nOSType);
//    redisValue["gender"]             = to_string((int)baseinfo.nGender);
//    redisValue["headId"]             = to_string((int)baseinfo.nHeadId);
//    redisValue["headboxId"]          = to_string((int)baseinfo.nHeadboxId);
//    redisValue["vip"]                = to_string((int)baseinfo.nVipLevel);
//    redisValue["vip2"]               = to_string((int)baseinfo.nVipLevel);
//    redisValue["temp"]               = to_string((int)baseinfo.nTemp);
//    redisValue["manager"]            = to_string((int)baseinfo.nIsManager);
//    redisValue["superAccount"]       = to_string((int)baseinfo.nIsSuperAccount);

//    redisValue["totalRecharge"]      = to_string((double)baseinfo.nTotalRecharge);
//    redisValue["score"]              = to_string((double)baseinfo.nUserScore);
//    redisValue["bankScore"]          = to_string((double)baseinfo.nBankScore);
//    redisValue["chargeAmount"]       = to_string((double)baseinfo.nChargeAmount);
//    redisValue["loginTime"]          = to_string(baseinfo.nLoginTime);
//    redisValue["gameStartTime"]      = to_string(baseinfo.nGameStartTime);

//    redisValue["headUrl"]            = std::string(baseinfo.szHeadUrl);
//    redisValue["account"]            = std::string(baseinfo.szAccount);
//    redisValue["nickName"]           = std::string(baseinfo.szNickName);
////    redisValue["regIp"]              = std::string(baseinfo.szIp);
//    redisValue["loginIp"]            = std::string(baseinfo.szIp);

////    std::string strLocation = Landy::Crypto::BufferToHexString((unsigned char*)baseinfo.szLocation, strlen(baseinfo.szLocation));
////    redisValue["loginLocation"]           = strLocation; //std::string(baseinfo.szLocation);

//    redisValue["loginLocation"]      = std::string(baseinfo.szLocation);

//    redisValue["password"]           = std::string(baseinfo.szPassword);
//    redisValue["dynamicPassword"]    = std::string(baseinfo.szDynamicPass);
//    redisValue["bankPassword"]       = std::string(baseinfo.szBankPassword);

//    redisValue["mobileNum"]          = std::string(baseinfo.szMobileNum);
//    redisValue["regMachineType"]     = std::string(baseinfo.szMachineType);
//    redisValue["regMachineSerial"]   = std::string(baseinfo.szMachineSerial);

//    redisValue["aliPayAccount"]      = std::string(baseinfo.szAlipayAccount);
//    redisValue["aliPayName"]         = std::string(baseinfo.szAlipayName);

//    redisValue["bankCardNum"]        = std::string(baseinfo.szBankCardNum);
//    redisValue["bankCardName"]       = std::string(baseinfo.szBankCardName);

//
//    return hmset(key, redisValue);
//}

//bool RedisClient::GetUserLoginInfo(int64_t userId, Global_UserBaseInfo& baseinfo)
//{
//    bool ok = false;
//    do
//    {
//        redis::RedisValue redisValue;
//        std::string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
//        std::string fields[] = {
//            "userId","proxyId","bindProxyId","gem","platformId","channelId", "ostype", "gender","headId",
//            "headboxId","vip","temp","manager","superAccount","totalRecharge","score","bankScore","chargeAmount", "loginTime", "gameStartTime",
//            "headUrl","account","nickName","loginIp","uuid","loginLocation","password","dynamicPassword",
//            "bankPassword","mobileNum","regMachineType","regMachineSerial","aliPayAccount","aliPayName","bankCardNum","bankCardName"
//        };

////        int  count = (sizeof(fields)/sizeof(fields[0]));
////        LOG_ERROR << "count = "<<count;
//        // try to get the special redis content from the set now.
//        if (exists(key) && hmget(key, fields, CountArray(fields), redisValue))
//        {
//            baseinfo.nUserId        = redisValue["userId"].asInt();
//            baseinfo.nPromoterId    = redisValue["proxyId"].asInt();
//            baseinfo.nBindPromoterId= redisValue["bindProxyId"].asInt();

//            baseinfo.nGem           = redisValue["gem"].asInt();
//            baseinfo.nPlatformId    = redisValue["platformId"].asInt();
//            baseinfo.nChannelId     = redisValue["channelId"].asInt();
//            baseinfo.nOSType        = redisValue["ostype"].asInt();
//            baseinfo.nGender        = redisValue["gender"].asInt();
//            baseinfo.nHeadId        = redisValue["headId"].asInt();
//            baseinfo.nHeadboxId     = redisValue["headboxId"].asInt();
//            baseinfo.nVipLevel      = redisValue["vip"].asInt();
//            baseinfo.nTemp          = redisValue["temp"].asInt();
//            baseinfo.nIsManager     = redisValue["manager"].asInt();
//            baseinfo.nIsSuperAccount = redisValue["superAccount"].asInt();

//            baseinfo.nTotalRecharge = redisValue["totalRecharge"].asDouble();
//            baseinfo.nUserScore     = redisValue["score"].asDouble();
//            baseinfo.nBankScore     = redisValue["bankScore"].asDouble();
//            baseinfo.nChargeAmount  = redisValue["chargeAmount"].asDouble();
//            baseinfo.nLoginTime     = redisValue["loginTime"].asInt64();
//            baseinfo.nGameStartTime = redisValue["gameStartTime"].asInt64();

//            snprintf(baseinfo.szHeadUrl,        sizeof(baseinfo.szHeadUrl),         "%s",   redisValue["headUrl"].asString().c_str());
//            snprintf(baseinfo.szAccount,        sizeof(baseinfo.szAccount),         "%s",   redisValue["account"].asString().c_str());
//            snprintf(baseinfo.szNickName,       sizeof(baseinfo.szNickName),        "%s",   redisValue["nickName"].asString().c_str());
//            snprintf(baseinfo.szIp,             sizeof(baseinfo.szIp),              "%s",   redisValue["loginIp"].asString().c_str());
//            snprintf(baseinfo.szLocation,       sizeof(baseinfo.szLocation),        "%s",   redisValue["loginLocation"].asString().c_str());

//            snprintf(baseinfo.szPassword,       sizeof(baseinfo.szPassword),        "%s",   redisValue["password"].asString().c_str());
//            snprintf(baseinfo.szDynamicPass,    sizeof(baseinfo.szDynamicPass),     "%s",   redisValue["dynamicPassword"].asString().c_str());
//            snprintf(baseinfo.szBankPassword,   sizeof(baseinfo.szBankPassword),    "%s",   redisValue["bankPassword"].asString().c_str());

//            snprintf(baseinfo.szMobileNum,      sizeof(baseinfo.szMobileNum),       "%s",   redisValue["mobileNum"].asString().c_str());
//            snprintf(baseinfo.szMachineType,    sizeof(baseinfo.szMachineType),     "%s",   redisValue["regMachineType"].asString().c_str());
//            snprintf(baseinfo.szMachineSerial,  sizeof(baseinfo.szMachineSerial),   "%s",   redisValue["regMachineSerial"].asString().c_str());

//            snprintf(baseinfo.szAlipayAccount,  sizeof(baseinfo.szAlipayAccount),   "%s",   redisValue["aliPayAccount"].asString().c_str());
//            snprintf(baseinfo.szAlipayName,     sizeof(baseinfo.szAlipayName),      "%s",   redisValue["aliPayName"].asString().c_str());

//            snprintf(baseinfo.szBankCardNum,    sizeof(baseinfo.szBankCardNum),     "%s",   redisValue["bankCardNum"].asString().c_str());
//            snprintf(baseinfo.szBankCardName,   sizeof(baseinfo.szBankCardName),    "%s",   redisValue["bankCardName"].asString().c_str());

//            ok = true;
//        }
//    }while (0);

//
//    return ok;
//}


