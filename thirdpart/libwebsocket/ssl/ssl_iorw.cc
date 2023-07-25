#include <libwebsocket/ssl.h>

static inline void memZero(void* p, size_t n)
{
	memset(p, 0, n);
}

//implicit_cast<ToType>(expr)
template<typename To, typename From>
inline To implicit_cast(From const& f)
{
	return f;
}

// use like this: down_cast<T*>(foo);
// so we only accept pointers
template<typename To, typename From>     
inline To down_cast(From* f)                     
{
	if (false)
	{
		implicit_cast<From*, To>(0);
	}
	assert(f == NULL || dynamic_cast<To>(f) != NULL);
	return static_cast<To>(f);
}

namespace muduo {
	namespace net {
		namespace ssl {

			///
			/// @code
			/// +-------------------+------------------+------------------+
			/// | prependable bytes |  readable bytes  |  writable bytes  |
			/// |                   |     (CONTENT)    |                  |
			/// +-------------------+------------------+------------------+
			/// |                   |                  |                  |
			/// 0      <=      readerIndex   <=   writerIndex    <=     size
			/// @endcode
			///
			ssize_t SSL_read(SSL* ssl, IBytesBuffer* buf, int* savedErrno) {
				assert(buf->writableBytes() >= 0);
				//SSL_pending = 0
				//_LOG_DEBUG("SSL_pending = %d", SSL_pending(ssl));
				//_LOG_DEBUG("begin {{{");
				ssize_t n = 0;
				do {
					//make sure that writable > 0
					if (buf->writableBytes() == 0) {
						buf->ensureWritableBytes(implicit_cast<size_t>(4096));
					}
#if 0 //test
					const size_t writable = 5;
					if (buf->writableBytes() < writable) {
						buf->ensureWritableBytes(implicit_cast<size_t>(writable));
					}
#else
					const size_t writable = buf->writableBytes();
#endif
					int rc = ::SSL_read(ssl, buf->beginWrite(), writable);
					if (rc > 0) {
						assert(::SSL_get_error(ssl, rc) == 0);
					}
					//returns the number of bytes which are available inside ssl for immediate read
					//make sure that call it after SSL_read is called
					size_t left = (size_t)::SSL_pending(ssl);
					//_LOG_DEBUG("rc = %d left = %lld err = %d",
					//    rc, left, SSL_get_error(ssl, rc));
					if (rc > 0) {
						assert(::SSL_get_error(ssl, rc) == 0);
						//IBytesBuffer::SSL_read() been called first time
						n += (ssize_t)rc;
						buf->hasWritten(rc);
						if (left > 0) {
							const size_t writable = buf->writableBytes();
							if (implicit_cast<size_t>(left) > writable) {
								buf->ensureWritableBytes(implicit_cast<size_t>(left));
							}
							continue;
						}
#if 0
						const char* c = "HTTP/1.1 200 OK\r\nConnection: Close\r\n\r\n";
						muduo::net::SSL_write(ssl, c, strlen(c));
#endif
						//read all of buf then return
						assert(left == 0);
						break;
					}
					else if (rc < 0) {
						int err = ::SSL_get_error(ssl, rc);
						_LOG_DEBUG("rc = %d err = %d errno = %d errmsg = %s",
							rc, err, errno, strerror(errno));
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*savedErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*savedErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*savedErrno = SSL_ERROR_SSL;
							break;
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*savedErrno = SSL_ERROR_ZERO_RETURN;
							//_LOG_DEBUG("SSL has been shutdown(%d)", err);
							break;
						default:
							if (errno != EAGAIN /*&&
								errno != EWOULDBLOCK &&
								errno != ECONNABORTED &&
								errno != EPROTO*/ &&
								errno != EINTR) {
								//*savedErrno = errno;
							}
							*savedErrno = err;
							break;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//IBytesBuffer::SSL_read() been called last time
						assert(left == 0);
						int err = ::SSL_get_error(ssl, rc);
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*savedErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*savedErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*savedErrno = SSL_ERROR_SSL;
							break;
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*savedErrno = SSL_ERROR_ZERO_RETURN;
							//_LOG_DEBUG("SSL has been shutdown(%d)", err);
							break;
							//Connection has been aborted by peer
						default:
							*savedErrno = err;
							//_LOG_DEBUG("Connection has been aborted(%d)", err);
							break;
						}
						break;
					}
				} while (true);
				//_LOG_DEBUG("end }}}");
				return n;
			}

			ssize_t SSL_write(SSL* ssl, void const* data, size_t len, int* savedErrno) {
				//_LOG_DEBUG("{{{");
				ssize_t left = (ssize_t)len;
				ssize_t n = 0;
				while (left > 0) {
					int rc = ::SSL_write(ssl, (char const*)data + n, left);
					if (rc > 0) {
						n += (ssize_t)rc;
						left -= (ssize_t)rc;
						//_LOG_DEBUG("rc = %d left = %d err = %d",
						//	rc, left, SSL_get_error(ssl, rc));
						assert(::SSL_get_error(ssl, rc) == 0);
					}
					else if (rc < 0) {
						//rc = -1 err = 1 errno = 0 errmsg = Success
						int err = ::SSL_get_error(ssl, rc);
						if (errno != EINTR || errno != EAGAIN) {
							_LOG_ERROR("rc = %d err = %d errno = %d errmsg = %s",
								rc, err, errno, strerror(errno));
						}
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*savedErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*savedErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*savedErrno = SSL_ERROR_SSL;
							break;
						default:
							if (errno != EAGAIN /*&&
								errno != EWOULDBLOCK &&
								errno != ECONNABORTED &&
								errno != EPROTO*/ &&
								errno != EINTR) {
								//*savedErrno = errno;
							}
							*savedErrno = err;
							break;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//assert(left == 0);
						int err = ::SSL_get_error(ssl, rc);
						switch (err)
						{
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*savedErrno = SSL_ERROR_ZERO_RETURN;
							_LOG_DEBUG("SSL has been shutdown(%d)", err);
							break;
							//Connection has been aborted by peer
						default:
							*savedErrno = err;
							_LOG_DEBUG("Connection has been aborted(%d)", err);
							break;
						}
						break;
					}
				}
				//_LOG_DEBUG("end }}}");
				return n;
			}

		}//namespace ssl
    } // namespace net
} // namespace muduo
