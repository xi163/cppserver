
#include "GlobalFunc.h"


int64_t GlobalFunc::GetCurrentStamp64(bool ismicrosec)
{
    // muduo::Timestamp start = muduo::Timestamp::now();
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1)); //boost::gregorian::Jan
    boost::posix_time::time_duration time_from_epoch;

    if (ismicrosec)
    {
        time_from_epoch = boost::posix_time::microsec_clock::universal_time() - epoch;
        return time_from_epoch.total_microseconds();
    }
    else
    {
        time_from_epoch = boost::posix_time::second_clock::universal_time() - epoch;
        return time_from_epoch.total_seconds();
    }
}

uint64_t GlobalFunc::RandomInt64(uint64_t min, uint64_t max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min,  max);
    return dis(gen);
}

uint16_t GlobalFunc::GetCheckSum(uint8_t* header, int size)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < size / 2; ++i)
        sum += *p++;
    if(size % 2)
        sum+=*(uint8_t*)p;
    return sum;
}

void GlobalFunc::SetCheckSum(packet::internal_prev_header_t *header)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < sizeof(packet::internal_prev_header_t) / 2 - 1; ++i)
        sum += *p++;
    *p = sum;
}

void GlobalFunc::trimstr(std::string & sourceStr)
{
    if(!sourceStr.empty())
    {
        sourceStr.erase(0,sourceStr.find_first_not_of(" "));
        sourceStr.erase(sourceStr.find_last_not_of(" ") + 1);
    }
}
void GlobalFunc::replaceChar(std::string &srcStr, char mark)
{
    std::string::size_type startpos = srcStr.find(mark);
    while (startpos != std::string::npos)
    {
        srcStr.replace(startpos, 1, ""); //实施替换，注意后面一定要用""引起来，表示字符串
        startpos = srcStr.find(mark);        //找到'\n'的位置
    }
}


bool GlobalFunc::CheckCheckSum(packet::internal_prev_header_t *header)
{
    uint16_t sum = 0;
    uint16_t *p = (uint16_t*)header;
    for(int i = 0; i < sizeof(packet::internal_prev_header_t) / 2 - 1; ++i)
        sum += *p++;
    return *p == sum;
}

int GlobalFunc::getIP(std::string netName, std::string &strIP)
{
#define BUF_SIZE 1024
    int sock_fd;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUF_SIZE] = {0};
    int num;
    int i;

    sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if ( sock_fd < 0 )
        return -1;

    conf.ifc_len = BUF_SIZE;
    conf.ifc_buf = buff;

    if ( ioctl(sock_fd, SIOCGIFCONF, &conf) < 0 )
    {
        close(sock_fd);
        return -1;
    }

    num = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;

    for(i = 0; i < num; i++)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);

        if ( ioctl(sock_fd, SIOCGIFFLAGS, ifr) < 0 )
        {
                close(sock_fd);
                return -1;
        }

        if ( (ifr->ifr_flags & IFF_UP) && strcmp(netName.c_str(),ifr->ifr_name) == 0 )
        {
                strIP = inet_ntoa(sin->sin_addr);
                close(sock_fd);

                return 0;
        }

        ifr++;
    }

    close(sock_fd);

    return -1;
}

std::string GlobalFunc::Uint32IPToString(uint32_t ip)
{
    sockaddr_in addr;
    bzero(&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;

    std::string strIP = inet_ntoa(addr.sin_addr);
    return strIP;
}

//URLEncode  URLDecode
uint8_t GlobalFunc::ToHex(uint8_t x)
{
    return  x > 9 ? x + 55 : x + 48;
}

uint8_t GlobalFunc::FromHex(uint8_t x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    //else
    //    assert(0);
    return y;
}

std::string GlobalFunc::converToHex(uint8_t const* hex, int size,int from)
{
    std::string strValue;
    char buf[32]={0};
    for(int i=from;i<size+from;i++)
    {
        snprintf(buf,sizeof(buf),"%02X", (unsigned char)hex[i]);
        strValue += buf;
    }

    return strValue;
}

std::string GlobalFunc::UrlEncode(std::string const& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~') ||
            (str[i] == '=') ||
            (str[i] == '!') ||
            (str[i] == '*') ||
            (str[i] == '\'') ||
            (str[i] == '(') ||
            (str[i] == ')') ||
            (str[i] == '&')  )
            strTemp += str[i];
        else if (str[i] == ' ')
            strTemp += "+";
        else
        {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] % 16);
        }
    }
    return strTemp;
}

std::string GlobalFunc::UrlDecode(std::string const& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+') strTemp += ' ';
        else if (str[i] == '%')
        {
            //assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high*16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}

void GlobalFunc::string_replace(std::string &strBig, const std::string &strSrc, const std::string &strDst)
{
    std::string::size_type pos = 0;
    std::string::size_type srcLen = strSrc.size();
    std::string::size_type dstLen = strDst.size();

    while( (pos=strBig.find(strSrc, pos)) != std::string::npos )
    {
        strBig.replace( pos, srcLen, strDst );
        pos += dstLen;
    }
}

//int GlobalFunc::curlSendSMS(string url, string content, bool bJson)
//{
//    CURL *curl;
//    CURLcode code, res;
//
////    content = UrlEncode(content);
////    LOG_INFO << "CONTENT Encode:"<<content;
//
//    struct curl_slist* headers = NULL;
//    if(bJson)
//        headers = curl_slist_append(headers, "Content-Type:application/json");
//    headers = curl_slist_append(headers, "charset=utf-8");
//
//    code = curl_global_init(CURL_GLOBAL_ALL);
//    curl = curl_easy_init();
//
//    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 6);
//    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
//    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
//    curl_easy_setopt(curl, CURLOPT_POST, 1);
//    res = curl_easy_perform(curl);
//    curl_easy_cleanup(curl);
//    curl_global_cleanup();
//
//    return res;
//}

std::string GlobalFunc::clearDllPrefix(std::string dllname)
{
    // drop the special ./ prefix path.
    size_t nprefix = dllname.find("./");
    if (0 == nprefix)
    {
        dllname.replace(nprefix,2,"");
    }

    // drop the lib prefix name now.
    nprefix  = dllname.find("lib");
    if (0 == nprefix)
    {
        dllname.replace(nprefix, 3, "");
    }

    // drop the .so extension name.
    nprefix = dllname.find(".so");
    if (std::string::npos != nprefix)
    {
        dllname.replace(nprefix,3,"");
    }

    return (dllname);
}

