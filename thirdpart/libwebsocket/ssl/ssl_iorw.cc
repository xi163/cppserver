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
			ssize_t SSL_read(SSL* ssl, IBytesBuffer* buf, int* saveErrno) {
				ASSERT(buf->writableBytes() >= 0);
				//SSL_pending = 0
				//Debugf("SSL_pending = %d", SSL_pending(ssl));
				//Debugf("begin {{{");
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
						ASSERT(::SSL_get_error(ssl, rc) == 0);
					}
					//returns the number of bytes which are available inside ssl for immediate read
					//make sure that call it after SSL_read is called
					size_t left = (size_t)::SSL_pending(ssl);
					//Debugf("rc = %d left = %lld err = %d",
					//    rc, left, SSL_get_error(ssl, rc));
					if (rc > 0) {
						ASSERT(::SSL_get_error(ssl, rc) == 0);
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
						ASSERT(left == 0);
						break;
					}
					else if (rc < 0) {
						int err = ::SSL_get_error(ssl, rc);
						Debugf("rc = %d err = %d errno = %d errmsg = %s",
							rc, err, errno, strerror(errno));
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*saveErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*saveErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*saveErrno = SSL_ERROR_SSL;
							break;
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*saveErrno = SSL_ERROR_ZERO_RETURN;
							Debugf("SSL has been shutdown(%d)", err);
							break;
						default:
							if (errno != EAGAIN /*&&
								errno != EWOULDBLOCK &&
								errno != ECONNABORTED &&
								errno != EPROTO*/ &&
								errno != EINTR) {
								//*saveErrno = errno;
							}
							*saveErrno = err;
							break;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//IBytesBuffer::SSL_read() been called last time
						ASSERT(left == 0);
						int err = ::SSL_get_error(ssl, rc);
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*saveErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*saveErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*saveErrno = SSL_ERROR_SSL;
							break;
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*saveErrno = SSL_ERROR_ZERO_RETURN;
							Debugf("SSL has been shutdown(%d)", err);
							break;
							//Connection has been aborted by peer
						default:
							*saveErrno = err;
							Debugf("Connection has been aborted(%d)", err);
							break;
						}
						break;
					}
				} while (true);
				//Debugf("end }}}");
				return n;
			}

			ssize_t SSL_write(SSL* ssl, void const* data, size_t len, int* saveErrno) {
				//Debugf("{{{");
				ssize_t left = (ssize_t)len;
				ssize_t n = 0;
				while (left > 0) {
					int rc = ::SSL_write(ssl, (char const*)data + n, left);
					if (rc > 0) {
						n += (ssize_t)rc;
						left -= (ssize_t)rc;
						//Debugf("rc = %d left = %d err = %d",
						//	rc, left, SSL_get_error(ssl, rc));
						ASSERT(::SSL_get_error(ssl, rc) == 0);
					}
					else if (rc < 0) {
						//rc = -1 err = 1 errno = 0 errmsg = Success
						int err = ::SSL_get_error(ssl, rc);
						if (errno != EINTR || errno != EAGAIN) {
							Errorf("rc = %d err = %d errno = %d errmsg = %s",
								rc, err, errno, strerror(errno));
						}
						switch (err)
						{
						case SSL_ERROR_WANT_READ:
							*saveErrno = SSL_ERROR_WANT_READ;
							break;
						case SSL_ERROR_WANT_WRITE:
							*saveErrno = SSL_ERROR_WANT_WRITE;
							break;
						case SSL_ERROR_SSL:
							*saveErrno = SSL_ERROR_SSL;
							break;
						default:
							if (errno != EAGAIN /*&&
								errno != EWOULDBLOCK &&
								errno != ECONNABORTED &&
								errno != EPROTO*/ &&
								errno != EINTR) {
								//*saveErrno = errno;
							}
							*saveErrno = err;
							break;
						}
						break;
					}
					else /*if (rc == 0)*/ {
						//ASSERT(left == 0);
						int err = ::SSL_get_error(ssl, rc);
						switch (err)
						{
							//SSL has been shutdown
						case SSL_ERROR_ZERO_RETURN:
							*saveErrno = SSL_ERROR_ZERO_RETURN;
							Debugf("SSL has been shutdown(%d)", err);
							break;
							//Connection has been aborted by peer
						default:
							*saveErrno = err;
							Debugf("Connection has been aborted(%d)", err);
							break;
						}
						break;
					}
				}
				//Debugf("end }}}");
				return n;
			}

		}//namespace ssl
    } // namespace net
} // namespace muduo
