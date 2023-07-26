#ifndef INCLUDE_MYAESECB_H
#define INCLUDE_MYAESECB_H

#include "Logger/src/Macro.h"

namespace Crypto {
	
	//AES_ECBEncrypt AES加密
	std::string AES_ECBEncrypt(std::string const& data, std::string const& key);
	
	//AES_ECBDecrypt AES解密
	std::string AES_ECBDecrypt(std::string const& data, std::string const& key);

	std::string Des_Encrypt(std::string const& data, char* key);

	std::string Des_Decrypt(std::string const& data, const char* key);
}

#endif