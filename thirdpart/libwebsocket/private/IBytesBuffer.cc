#include "libwebsocket/IBytesBuffer.h"

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
		ssize_t IBytesBuffer::readFull(int sockfd, IBytesBuffer* buf, ssize_t& rc, int* saveErrno) {
			ASSERT(buf->writableBytes() >= 0);
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
				/*const ssize_t */rc = ::read(sockfd, buf->beginWrite(), writable);
				//*saveErrno = errno;
				if (rc > 0) {
					//
					//rc > 0 errno = EAGAIN(11)
					//rc > 0 errno = 0
					//
					//只要可读(内核buf中还有数据)，就一直读，直到返回0，或者errno = EAGAIN
					n += (ssize_t)rc;
					buf->hasWritten(rc);
#if 0
					//make sure that reset errno = 0 after callback
					if (errno == EAGAIN ||
						errno == EINTR) {
						break;
					}
#endif
					continue;
				}
				else if (rc < 0) {
					*saveErrno = errno;
					//
					//rc = -1 errno = EAGAIN(11)
					//rc = -1 errno = 0
					//
					if (errno != EAGAIN /*&&
						errno != EWOULDBLOCK &&
						errno != ECONNABORTED &&
						errno != EPROTO*/ &&
						errno != EINTR) {
						Errorf("rc = %d errno = %d errmsg = %s",
							rc, errno, strerror(errno));
					}
					break;
				}
				else /*if (rc == 0)*/ {
					//
					//rc = 0 errno = EAGAIN(11)
					//rc = 0 errno = 0 peer close
					//
					//Connection reset by peer
					Tracef("Connection reset by peer rc = %d errno = %d errmsg = %s",
						rc, errno, strerror(errno));
					break;
				}
			} while (true);
			return n;
		}//readFull
		
		ssize_t IBytesBuffer::writeFull(int sockfd, void const* data, size_t len, ssize_t& rc, int* saveErrno) {
			ssize_t left = (ssize_t)len;
			ssize_t n = 0;
			while (left > 0) {
				/*const ssize_t */rc = ::write(sockfd, (char const*)data + n, left);
				//*saveErrno = errno;
				if (rc > 0) {
					//
					//rc > 0 errno = EAGAIN(11)
					//rc > 0 errno = 0
					//rc > 0 errno = EINPROGRESS(115)
					//
					//只要可写(内核buf还有空间且用户待写数据还未写完)，就一直写，直到数据发送完，或者errno = EAGAIN
					n += (ssize_t)rc;
					left -= (ssize_t)rc;
					//Tracef("rc = %d left = %d errno = %d errmsg = %s",
					//	rc, left, errno, strerror(errno));
					continue;
				}
				else if (rc < 0) {
					*saveErrno = errno;
					//
					//rc = -1 errno = EAGAIN(11)
					//rc = -1 errno = 0
					//
					if (errno != EAGAIN /*&&
						errno != EWOULDBLOCK &&
						errno != ECONNABORTED &&
						errno != EPROTO*/ &&
						errno != EINTR) {
						Errorf("rc = %d left = %d errno = %d errmsg = %s",
							rc, left, errno, strerror(errno));
					}
					break;
				}
				else /*if (rc == 0)*/ {
					//
					//rc = 0 errno = EAGAIN(11)
					//rc = 0 errno = 0 peer close
					//
					//ASSERT(left == 0);
					//Connection reset by peer
					Tracef("Connection reset by peer rc = %d errno = %d errmsg = %s",
						rc, errno, strerror(errno));
					break;
				}
			}
			return n;
		}//writeFull

    } // namespace net
} // namespace muduo