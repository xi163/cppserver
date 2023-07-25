#ifndef INCLUDE_GLOBALFUNC_H
#define INCLUDE_GLOBALFUNC_H

#include "public/Inc.h"

class GlobalFunc
{
public:
    GlobalFunc() = default;

public:
    static uint64_t RandomInt64(uint64_t min, uint64_t max);
    static uint16_t GetCheckSum(uint8_t* header, int size);

    static void SetCheckSum(packet::internal_prev_header_t *header);
    static bool CheckCheckSum(packet::internal_prev_header_t *header);
    static int getIP(std::string netName, std::string &strIP);
    static std::string Uint32IPToString(uint32_t ip);
    static uint8_t ToHex(uint8_t x);
    static uint8_t FromHex(uint8_t x);
    static std::string converToHex(uint8_t const* hex, int size,int from = 0);
    static std::string UrlEncode(std::string const& str);
    static std::string UrlDecode(std::string const& str);
    static void string_replace(std::string &strBig, const std::string &strSrc, const std::string &strDst);
   // static int curlSendSMS(std::string url, std::string content, bool bJson);
    static std::string clearDllPrefix(std::string dllName);
    static void trimstr(std::string & sourceStr);
    static void replaceChar(std::string &srcStr, char mark);
    static int64_t GetCurrentStamp64(bool ismicrosec = true);
};

#endif