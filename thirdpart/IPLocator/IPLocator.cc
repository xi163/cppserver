
#include "Logger/src/utils/utils.h"

#include "IPLocator.h"

CIPLocator::CIPLocator() {
}

CIPLocator::CIPLocator(char const* file) {
    Open(file);
}

//打开数据库文件
bool CIPLocator::Open(char const* file) {
    fp_ = fopen(file, "rb");
    if (!fp_) {
        return false;
    }
    // IP头由两个十六进制4字节偏移量构成，分别为索引开始，和索引结束
    start_ = GetValue4(0);
    end_ = GetValue4(4);
    return true;
}

CIPLocator::~CIPLocator() {
    ::fclose(fp_);
}

//根据IP地址字符串返回其十六进制值（4字节）
unsigned long CIPLocator::IpString2IpValue(char const* ip) const {
    if (!IsRightIpString(ip)) {
        return 0;
    }
    int nNum = 0;            // 每个段数值
    char const*pBeg = ip;
    char const*pPos = NULL;
    unsigned long ulIp = 0; // 整个IP数值

    pPos = strchr(ip, '.');
    while (pPos != NULL) {
        nNum = atoi(pBeg);
        ulIp += nNum;
        ulIp *= 0x100;
        pBeg = pPos + 1;
        pPos = strchr(pBeg, '.');
    }

    nNum = atoi(pBeg);
    ulIp += nNum;
    return ulIp;
}

//根据ip值获取字符串（由点分割）
void CIPLocator::IpValue2IpString(unsigned long ipval, char *ip, size_t size) const {
    if (!ip) {
        return;
    }

#ifdef _windows_
    _snprintf(ip, size, "%d.%d.%d.%d", (ipval & 0xFF000000) >> 24,
        (ipval & 0x00FF0000) >> 16, (ipval & 0x0000FF00) >> 8, ipval & 0x000000FF);
#else
    snprintf(ip, size, "%lu.%lu.%lu.%lu", (ipval & 0xFF000000) >> 24,
        (ipval & 0x00FF0000) >> 16, (ipval & 0x0000FF00) >> 8, ipval & 0x000000FF);
#endif

    ip[size - 1] = 0;
}

//根据指定IP(十六进制值)，返回其在索引段中的位置(索引)
//start和end可以指定搜索范围 均为0表示搜索全部
unsigned long CIPLocator::SearchIp(unsigned long ipval, unsigned long start, unsigned long end) const {
    if (!fp_) {
        return 0;
    }

    if (0 == start) {
        start = start_;
    }

    if (0 == end) {
        end = end_;
    }

    unsigned long indexLeft = start;
    unsigned long indexRight = end;

    // 先除后乘是为了保证mid指向一个完整正确的索引
    unsigned long indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;

    // 起始Ip地址(如172.23.0.0),他和Ip记录中的Ip地址(如172.23.255.255)构成一个Ip范围，在这个范围内的Ip都可以由这条索引来获取国家、地区
    unsigned long ipHeader = 0;

    do {
        ipHeader = GetValue4(indexMid);
        if (ipval < ipHeader) {
            indexRight = indexMid;
            indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;
        }
        else {
            indexLeft = indexMid;
            indexMid = (indexRight - indexLeft) / INDEX_LENGTH / 2 * INDEX_LENGTH + indexLeft;
        }
    } while (indexLeft < indexMid);// 注意是根据mid来进行判断

    // 只要符合范围就可以，不需要完全相等
    return indexMid;
}

unsigned long CIPLocator::SearchIp(char const*ip, unsigned long start, unsigned long end) const {
    if (!IsRightIpString(ip)) {
        return 0;
    }
    return SearchIp(IpString2IpValue(ip), start, end);
}

//从指定位置获取一个十六进制的数 (读取3个字节， 主要用于获取偏移量， 与效率紧密相关的函数，尽可能优化）
unsigned long CIPLocator::GetValue3(unsigned long start) const {
    if (!fp_) {
        return 0;
    }

    int val[3];
    unsigned long value = 0;

    fseek(fp_, start, SEEK_SET);
    for (int i = 0; i < 3; ++i) {
        // 过滤高位，一次读取一个字符
        val[i] = fgetc(fp_) & 0x000000FF;
    }

    for (int j = 2; j >= 0; --j) {
        // 因为读取多个16进制字符，叠加
        value = value * 0x100 + val[j];
    }
    return value;
}

//从指定位置获取一个十六进制的数 (读取4个字节， 主要用于获取IP值， 与效率紧密相关的函数，尽可能优化）
unsigned long CIPLocator::GetValue4(unsigned long start) const {
    if (!fp_) {
        return 0;
    }

    int val[4];
    unsigned long value = 0;

    fseek(fp_, start, SEEK_SET);
    for (int i = 0; i < 4; ++i) {

        // 过滤高位，一次读取一个字符
        val[i] = fgetc(fp_) & 0x000000FF;
    }

    for (int j = 3; j >= 0; --j) {

        // 因为读取多个16进制字符，叠加
        value = value * 0x100 + val[j];
    }
    return value;
}

//从指定位置获取字符串
unsigned long CIPLocator::GetString(std::string& str, unsigned long start) const {
    if (!fp_) {
        return 0;
    }

    str.erase(0, str.size());

    fseek(fp_, start, SEEK_SET);
    
    int ch = fgetc(fp_);
    unsigned long count = 1;
    
    // 读取字符串，直到遇到0x00为止
    while (ch != 0x00) {

        // 依次放入用来存储的字符串空间中
        str += static_cast<char>(ch);
        ++count;
        ch = fgetc(fp_);
    }

    // 返回字符串长度
    return count;
}

//通过指定的偏移量来获取ip记录中的国家名和地区名，偏移量可由索引获取
//offset为Ip记录偏移量
void CIPLocator::GetAddressByOffset(unsigned long offset, std::string& country, std::string& location) const {
    if (!fp_) {
        return;
    }

    // 略去4字节Ip地址
    offset += IP_LENGTH;
    fseek(fp_, offset, SEEK_SET);

    // 读取首地址的值
    int val = (fgetc(fp_) & 0x000000FF);
    unsigned long countryOffset = 0;    // 真实国家名偏移
    unsigned long locationOffset = 0; // 真实地区名偏移
    
    // 为了节省空间，相同字符串使用重定向，而不是多份拷贝
    if (REDIRECT_MODE_1 == val) {

        // 重定向1类型
        unsigned long redirect = GetValue3(offset + 1); // 重定向偏移

        fseek(fp_, redirect, SEEK_SET);

        if ((fgetc(fp_) & 0x000000FF) == REDIRECT_MODE_2) {

            // 混合类型1，重定向1类型进入后遇到重定向2类型
            // 0x01 1字节
            // 偏移量 3字节 -----> 0x02 1字节
            //                     偏移量 3字节 -----> 国家名
            //                     地区名
            countryOffset = GetValue3(redirect + 1);
            GetString(country, countryOffset);
            locationOffset = redirect + 4;
        }
        else {

            // 单纯的重定向模式1
            // 0x01 1字节
            // 偏移量 3字节 ------> 国家名
            //                      地区名
            countryOffset = redirect;
            locationOffset = redirect + GetString(country,
                countryOffset);
        }
    }
    else if (REDIRECT_MODE_2 == val) {
        // 重定向2类型
        // 0x02 1字节
        // 国家偏移 3字节 -----> 国家名
        // 地区名
        countryOffset = GetValue3(offset + 1);
        GetString(country, countryOffset);

        locationOffset = offset + 4;
    }
    else {
        // 最简单的情况 没有重定向
        // 国家名
        // 地区名
        countryOffset = offset;
        locationOffset = countryOffset + GetString(country,
            countryOffset);
    }

    // 读取地区
    fseek(fp_, locationOffset, SEEK_SET);
    if ((fgetc(fp_) & 0x000000FF) == REDIRECT_MODE_2
        || (fgetc(fp_) & 0x000000FF) == REDIRECT_MODE_1) {

        // 混合类型2(最复杂的情形，地区也重定向)
        // 0x01 1字节
        // 偏移量 3字节 ------> 0x02 1字节
        //                      偏移量 3字节 -----> 国家名
        //                      0x01 or 0x02 1字节
        //                      偏移量 3字节 ----> 地区名 偏移量为0表示未知地区
        locationOffset = GetValue3(locationOffset + 1);
    }

    GetString(location, locationOffset);
}

//根据十六进制ip获取国家名地区名
void CIPLocator::GetAddressByIp(unsigned long ipval, std::string& country, std::string& location) const {
    
    unsigned long indexOffset = SearchIp(ipval);
    unsigned long recordOffset = GetValue3(indexOffset + IP_LENGTH);

    GetAddressByOffset(recordOffset, country, location);

    country = boost::locale::conv::to_utf<char>(country, "gb2312");
    location = boost::locale::conv::to_utf<char>(location, "gb2312");

    std::string oldstr_province("省");
    std::string oldstr_city("市");
    country = boost::trim_copy(country);
    boost::replace_first(country, oldstr_province, "");
    boost::replace_first(country, oldstr_city, "");
    if (country.length()>1024)
        country = country.substr(0,1024);
}

//根据ip字符串获取国家名地区名
void CIPLocator::GetAddressByIp(char const*ip, std::string& country, std::string& location) const {
    if (!IsRightIpString(ip)) {
        return;
    }
    GetAddressByIp(IpString2IpValue(ip), country, location);
}

//将ip数据导出，start和end界定导出范围， 可通过SearchIp来获取
unsigned long CIPLocator::OutputData(char const*file, unsigned long start, unsigned long end) const {
    if (!fp_ || !file) {
        return 0;
    }

    FILE *fp = fopen(file, "wb");

    if (!fp) {
        return 0;
    }

    if (0 == start) {
        start = start_;
    }

    if (0 == end) {
        end = end_;
    }

    
    char endIp[255];
    char startIp[255];
    std::string country;
    std::string location;
    unsigned long count = 0;
    unsigned long _end = 0;
    unsigned long _start = 0;
    
    for (unsigned long i = start; i < end; i += INDEX_LENGTH) {

        // 获取IP段的起始IP和结束IP， 起始IP为索引部分的前4位16进制
        // 结束IP在IP信息部分的前4位16进制中，靠索引部分指定的偏移量找寻
        _start = GetValue4(i);
        _end = GetValue4(GetValue3(i + IP_LENGTH));

        // 导出IP信息，格式是 起始IP/t结束IP/t国家位置/t地域位置/n
        IpValue2IpString(_start, startIp, sizeof(startIp));
        IpValue2IpString(_end, endIp, sizeof(endIp));
        GetAddressByOffset(GetValue3(i + IP_LENGTH), country,
            location);
        fprintf(fp, "%s/t%s/t%s/t%s/n", startIp, endIp,
            country.c_str(), location.c_str());
        count++;
    }

    fclose(fp);

    // 返回导出总条数
    return count;
}

//通过ip值界定导出范围
unsigned long CIPLocator::OutputDataByIp(char const*file, unsigned long start, unsigned long end) const {
    /*unsigned long */start = SearchIp(start);
    /*unsigned long */end = SearchIp(end);
    return OutputData(file, start, end);
}

//通过ip字符串界定导出范围
unsigned long CIPLocator::OutputDataByIp(char const*file, char const* startIp, char const* endIp) const {
    if (!IsRightIpString(startIp) || !IsRightIpString(endIp)) {
        return 0;
    }
    
    unsigned long end = IpString2IpValue(endIp);
    unsigned long start = IpString2IpValue(startIp);
    /*unsigned long */end = SearchIp(end);
    /*unsigned long */start = SearchIp(start);
    
    return OutputData(file, start, end);
}

//判断给定IP字符串是否是合法的ip地址
bool CIPLocator::IsRightIpString(char const* ip) const {
    if (!ip) {
        return false;
    }

    int nLen = (int)strlen(ip);
    if (nLen < 7) {

        // 至少包含7个字符"0.0.0.0"
        return false;
    }

    for (int i = 0; i < nLen; ++i) {
        if (!isdigit(ip[i]) && ip[i] != '.') {
            return false;
        }

        if (ip[i] == '.') {
            if (0 == i) {
                if (!isdigit(ip[i + 1])) {
                    return false;
                }
            }
            else if (nLen - 1 == i) {
                if (!isdigit(ip[i - 1])) {
                    return false;
                }
            }
            else {
                if (!isdigit(ip[i - 1]) || !isdigit(ip[i + 1])) {
                    return false;
                }
            }
        }
    }
    return true;
}
