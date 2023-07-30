#ifndef INCLUDE_ZLIB_GZIP_H
#define INCLUDE_ZLIB_GZIP_H

#include <zlib.h>

int gzip(Bytef const* src, uLongf srclen, Bytef *dst, uLongf &dstlen);
int gunzip(Bytef const* src, uLongf srclen, Bytef *dst, uLongf &dstlen);
int gunzip(Bytef const* src, uLongf srclen, Bytef * &dst, uLongf &dstlen, int g);

#endif