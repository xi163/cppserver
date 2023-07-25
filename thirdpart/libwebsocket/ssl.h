#ifndef _MUDUO_NET_SSL_H_
#define _MUDUO_NET_SSL_H_

#include "Logger/src/utils/utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/md5.h>
#include <openssl/x509v3.h>

#ifdef __cplusplus
}
#endif

#include <libwebsocket/IBytesBuffer.h>

namespace muduo {
	namespace net {
		namespace ssl {
		
			void SSL_CTX_Init(
				std::string const& cert_path,
				std::string const& private_key_path,
				std::string const& client_ca_cert_file_path = "",
				std::string const& client_ca_cert_dir_path = "");

			SSL_CTX* SSL_CTX_Get();

			void SSL_CTX_free();

			ssize_t SSL_read(SSL* ssl, IBytesBuffer* buf, int* savedErrno);

			ssize_t SSL_write(SSL* ssl, void const* data, size_t len, int* savedErrno);

			bool SSL_handshake(SSL_CTX* ctx, SSL*& ssl, int sockfd, int& saveErrno);

			void SSL_free(SSL*& ssl);

		};//namespace ssl
	};//namespace net
}; //namespace muduo

#endif
