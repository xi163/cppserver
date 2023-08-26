#ifndef INCLUDE_IPLOCATOR_H
#define INCLUDE_IPLOCATOR_H

#include "Logger/src/Macro.h"

const int INDEX_LENGTH = 7; // 一个索引包含4字节的起始IP和3字节的IP记录偏移，共7字节
const int IP_LENGTH = 4;
const int OFFSET_LENGTH = 3;

enum {
    REDIRECT_MODE_1 = 0x01, // 重定向模式1 偏移量后无地区名
    REDIRECT_MODE_2 = 0x02, // 重定向模式2 偏移量后有地区名
};

class CIPLocator {
public:
    CIPLocator();
    CIPLocator(char const* file);
    ~CIPLocator();

    // 获取ip国家名、地区名
    void GetAddressByIp(unsigned long ipval, std::string& country, std::string& location) const;
    void GetAddressByIp(char const* ip, std::string& country, std::string& location) const;
    void GetAddressByOffset(unsigned long offset, std::string& country, std::string& location) const;

    unsigned long GetString(std::string& str, unsigned long start) const;
    unsigned long GetValue3(unsigned long start) const;
    unsigned long GetValue4(unsigned long start) const;

    // 转换
    unsigned long IpString2IpValue(char const*ip) const;
    void IpValue2IpString(unsigned long ipval, char* ip, size_t size) const;
    bool IsRightIpString(char const* ip) const;

    // 输出数据
    unsigned long OutputData(char const* file, unsigned long start = 0, unsigned long end = 0) const;
    unsigned long OutputDataByIp(char const* file, unsigned long start, unsigned long end) const;
    unsigned long OutputDataByIp(char const* file, char const* startIp, char const* endIp) const;

    unsigned long SearchIp(unsigned long ipval, unsigned long start = 0, unsigned long end = 0) const;
    unsigned long SearchIp(char const* ip, unsigned long start = 0, unsigned long end = 0) const;

    bool Open(char const* file);
private:
    FILE *fp_;// IP数据库文件
    unsigned long start_;// 起始索引偏移
    unsigned long end_;// 结束索引偏移
};

#endif