/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/
#ifndef INCLUDE_SHA_H
#define INCLUDE_SHA_H

#include "Logger/src/Macro.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

namespace utils {

	typedef struct {
		unsigned int count[2];
		unsigned int state[5];
		unsigned char buffer[64];
	} SHA1_CTX;

	void SHA1Init(SHA1_CTX* context);
	void SHA1Update(SHA1_CTX* context, const unsigned char* data, unsigned int len);
	void SHA1Final(SHA1_CTX* context, unsigned char digest[20]);
	void SHA1Transform(unsigned int state[5], const unsigned char buffer[64]);

	char const* SHA1(char const* src, unsigned len, char dst[], int upper);
}

// #ifdef __cplusplus
// }
// #endif

#endif