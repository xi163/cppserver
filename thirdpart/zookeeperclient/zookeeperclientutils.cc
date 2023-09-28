#include "Logger/src/utils/utils.h"

#include "zookeeperclient/zookeeperclientutils.h"

ZookeeperClientUtils::ZookeeperClientUtils()
{

}

void ZookeeperClientUtils::printString(const std::string & name)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("{} ",name);
 //   cout<<"{ "<<name<<" }"<<endl;
}

void ZookeeperClientUtils::printStringPair(const std::pair<std::string, std::string> &stringPair)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("{}:{}",stringPair.first,stringPair.second);
 //   cout<<"{ "<<stringPair.first<<" }:"<<"{ "<<stringPair.second<<" }"<<endl;
}

void ZookeeperClientUtils::transStringVector2VectorString(const String_vector &children, std::vector<std::string> &vecString)
{
    for (int i = 0; i < children.count; i++)
    {
        vecString.push_back(children.data[i]);
    }
}

void ZookeeperClientUtils::printPathList(const std::vector<std::string> &pathVector)
{
    for_each(pathVector.begin(), pathVector.end(), ZookeeperClientUtils::printString);
}

void ZookeeperClientUtils::printPathValueList(const std::map<std::string, std::string> &pathValueMap)
{
    for_each(pathValueMap.begin(), pathValueMap.end(), ZookeeperClientUtils::printStringPair);
}


void ZookeeperClientUtils::transACLVector2VectorACL(const ACL_vector &aclVec, std::vector<ACL> &vectorACL)
{
    for (int i = 0; i < aclVec.count; i++)
    {
        vectorACL.push_back(aclVec.data[i]);
    }
}

const std::string ZookeeperClientUtils::watcherEventType2String(int type)
{
    switch(type)
    {
        case 0:
            return "ZOO_ERROR_EVENT";
        case CREATED_EVENT_DEF:
            return "ZOO_CREATED_EVENT";
        case DELETED_EVENT_DEF:
            return "ZOO_DELETED_EVENT";
        case CHANGED_EVENT_DEF:
            return "ZOO_CHANGED_EVENT";
        case CHILD_EVENT_DEF:
            return "ZOO_CHILD_EVENT";
        case SESSION_EVENT_DEF:
            return "ZOO_SESSION_EVENT";
        case NOTWATCHING_EVENT_DEF:
            return "ZOO_NOTWATCHING_EVENT";
    }
    return "INVALID_EVENT";
}


const std::string ZookeeperClientUtils::state2String(int state)
{
    switch(state)
    {
        case 0:
            return "ZOO_CLOSED_STATE";
        case CONNECTING_STATE_DEF:
            return "ZOO_CONNECTING_STATE";
        case ASSOCIATING_STATE_DEF:
            return "ZOO_ASSOCIATING_STATE";
        case CONNECTED_STATE_DEF:
            return "ZOO_CONNECTED_STATE";
        case EXPIRED_SESSION_STATE_DEF:
            return "ZOO_EXPIRED_SESSION_STATE";
        case AUTH_FAILED_STATE_DEF:
            return "ZOO_AUTH_FAILED_STATE";
    }
    return "INVALID_STATE";
}


