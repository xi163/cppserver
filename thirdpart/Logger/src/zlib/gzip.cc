#include "../../Macro.h"
#include "../../log/impl/LoggerImpl.h"

#include "gzip.h"

#if _windows_
#pragma comment(lib, "zlib.lib")
#endif

int gzip(Bytef const* src, uLongf srclen, Bytef *dst, uLongf &dstlen) {
	z_stream stream;
	int rc = 0;

	if (src && srclen > 0) {
		stream.zalloc = NULL;
		stream.zfree = NULL;
		stream.opaque = NULL;
		//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
		if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) return -1;
		stream.next_in = (Bytef *)src;
		stream.avail_in = srclen;
		stream.next_out = dst;
		stream.avail_out = dstlen;
		while (stream.avail_in != 0 && stream.total_out < dstlen) {
			if (deflate(&stream, Z_NO_FLUSH) != Z_OK) return -1;
		}
		if (stream.avail_in != 0) return stream.avail_in;
		for (;;) {
			if ((rc = deflate(&stream, Z_FINISH)) == Z_STREAM_END) break;
			if (rc != Z_OK) return -1;
		}
		if (deflateEnd(&stream) != Z_OK) return -1;
		dstlen = stream.total_out;
		return 0;
	}
	return -1;
}

int gunzip(Bytef const* src, uLongf srclen, Bytef *dst, uLongf &dstlen) {
	int rc = 0;
	z_stream stream = { 0 }; /* decompression stream */
	static char dummy_head[2] = {
		0x8 + 0x7 * 0x10,
		(((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
	};
	
	stream.zalloc = NULL;
	stream.zfree = NULL;
	stream.opaque = NULL;
	stream.next_in = (Bytef *)src;
	stream.avail_in = 0;
	stream.next_out = dst;
	
#if 0
	//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
	rc = inflateInit2(&stream, MAX_WBITS + 16);
#else
	rc = inflateInit2(&stream, 47);
#endif
	if (rc != Z_OK)
		return -1;

	while (stream.total_out < dstlen && stream.total_in < srclen) {
		stream.avail_in = stream.avail_out = 1; /* force small buffers */
		if ((rc = inflate(&stream, Z_NO_FLUSH)) == Z_STREAM_END) break;
		if (rc != Z_OK) {
			if (rc == Z_DATA_ERROR) {
				stream.next_in = (Bytef*)dummy_head;
				stream.avail_in = sizeof(dummy_head);
				if ((rc = inflate(&stream, Z_NO_FLUSH)) != Z_OK) {
					return -1;
				}
			}
			else return -1;
		}
	}
	if (inflateEnd(&stream) != Z_OK) return -1;
	dstlen = stream.total_out;
	return 0;
}

int gunzip(Bytef const* src, uLongf srclen, Bytef * &dst, uLongf &dstlen, int g) {
	dst = NULL;
	dstlen = 0;

	int rc;
	uLongf have;
	uLongf offset = 0;
	z_stream stream;

#ifndef HEAP__
	const uLongf OUTSIZE = 8192 * 10 * 12;
	Bytef out[OUTSIZE];
#else
	uLongf outsize = srclen/* * 10*/;
	Bytef * out = new Bytef[outsize];
#endif

	/*  allocate  inflate  state  */
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.next_in = Z_NULL;
	stream.avail_in = 0;
	
	if (g)
#if 0
		rc = inflateInit2(&stream, 47);
#else
		rc = inflateInit2(&stream, MAX_WBITS + 16);
#endif
	else
		rc = inflateInit(&stream);
	if (rc != Z_OK)
		return rc;

	stream.next_in = (Bytef *)src;
	stream.avail_in = srclen;

	/*  run  inflate()  on  input  until  output  buffer  not  full  */
	do {
		stream.next_out = out;
#ifndef HEAP__
		stream.avail_out = OUTSIZE;
#else
		stream.avail_out = outsize;
#endif
		rc = inflate(&stream, Z_NO_FLUSH);

		assert(rc != Z_STREAM_ERROR);    /*  state  not  clobbered  */

		switch (rc) {
		case  Z_NEED_DICT:
			rc = Z_DATA_ERROR;          /*  and  fall  through  */
		case  Z_DATA_ERROR:
		case  Z_MEM_ERROR:
			(void)inflateEnd(&stream);
#undef _DEBUG
#ifndef _DEBUG
#ifdef HEAP__
			delete[] out;
#endif
			return rc;
#else
			{
#ifndef HEAP__
				have = OUTSIZE - stream.avail_out;
#else
				have = outsize - stream.avail_out;
#endif
				dst = (Bytef *)realloc(dst, offset + have);
				memcpy(dst + offset, out, have);

				offset += have;

				dstlen = offset;
			}
#ifdef HEAP__
			delete[] out;
#endif
			return Z_OK;
#endif
		}
#ifndef HEAP__
		have = OUTSIZE - stream.avail_out;
#else
		have = outsize - stream.avail_out;
#endif
		dst = (Bytef *)realloc(dst, offset + have);
		memcpy(dst + offset, out, have);

		offset += have;

	} while (stream.avail_out == 0);

	/*  clean  up  and  return  */
	(void)inflateEnd(&stream);
	
//	memcpy(dst + offset, "\0", 1);
	assert(stream.total_out == offset);
	dstlen = offset;

#ifdef HEAP__
	delete[] out;
#endif

	return rc != Z_OK ? (rc == Z_STREAM_END ? Z_OK : Z_DATA_ERROR) : Z_OK;
}