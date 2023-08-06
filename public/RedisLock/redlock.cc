/* RedLock implement DLM(Distributed Lock Manager) with cpp.
 *
 * Copyright (c) 2014, jacketzhong <jacketzhong at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/Inc.h"

#include "redlock.h"

/* Turn the plain C strings into Sds strings */
static char **convertToSds(int count, char** args) {
    int j;
    char **sds = (char**)malloc(sizeof(char*)*count);
    for(j = 0; j < count; j++)
        sds[j] = sdsnew(args[j]);
    return sds;
}

namespace RedisLock {
	CLock::CLock() : m_validityTime(0), m_resource(NULL), m_val(NULL) {
	}

	CLock::~CLock() {
		sdsfree(m_resource);
		sdsfree(m_val);
	}

	int CRedLock::m_defaultRetryCount = 3;
	int CRedLock::m_defaultRetryDelay = 200;
	float CRedLock::m_clockDriftFactor = 0.01;

	// ----------------
	// init redlock
	// ----------------
	CRedLock::CRedLock() {
		Initialize();
	}

	// ----------------
	// release redlock
	// ----------------
	CRedLock::~CRedLock() {
		sdsfree(m_continueLockScript);
		sdsfree(m_unlockScript);
		close(m_fd);
		/* Disconnects and frees the context */
		for (int i = 0; i < (int)m_redisServer.size(); i++) {
			redisFree(m_redisServer[i]);
		}
	}

	// ----------------
	// Initialize the server: scripts...
	// ----------------
	bool CRedLock::Initialize() {
		m_continueLockScript = sdsnew("if redis.call('get', KEYS[1]) == ARGV[1] then redis.call('del', KEYS[1]) end return redis.call('set', KEYS[1], ARGV[2], 'px', ARGV[3], 'nx')");
		m_unlockScript = sdsnew("if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end");
		m_retryCount = m_defaultRetryCount;
		m_retryDelay = m_defaultRetryDelay;
		m_quoRum = 0;
		// open rand file
		m_fd = open("/dev/urandom", O_RDONLY);
		if (m_fd == -1) {
			//Open error handling
#if 0
			printf("Can't open file /dev/urandom\n");
#else
			_LOG_WARN("Can't open file /dev/urandom");
#endif
			exit(-1);
			return false;
		}
		return true;
	}

	// ----------------
	// auth redis server
	// ----------------
	static bool auth(redisContext* c, std::string const& passwd) {
		redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", passwd.c_str());
		if (reply) {
			if (reply->type == REDIS_REPLY_STATUS && reply->str && strcasecmp(reply->str, "OK") == 0) {
				return true;
			}
			_LOG_ERROR("%.*s", strlen(reply->str), reply->str);
			freeReplyObject(reply);
		}
		return false;
	}
	
	// ----------------
	// add redis server
	// ----------------
	bool CRedLock::AddServerUrl(const char *ip, const int port, std::string const& passwd) {
		redisContext *c = NULL;
		struct timeval timeout = { 1, 500000 }; // 1.5 seconds
		c = redisConnectWithTimeout(ip, port, timeout);
		if (c) {
			if (!passwd.empty()) {
				if (auth(c, passwd)) {
					_LOG_WARN("%.*s:%d succ", strlen(ip), ip, port);
					m_redisServer.push_back(c);
				}
				else {
					_LOG_ERROR("%.*s:%d failed", strlen(ip), ip, port);
				}
			}
			else {
				m_redisServer.push_back(c);
			}
		}
		else {
			_LOG_ERROR("redisConnectWithTimeout %.*s:%d", strlen(ip), ip, port);
		}
		m_quoRum = (int)m_redisServer.size() / 2 + 1;
		return true;
	}

	// ----------------
	// set retry
	// ----------------
	void CRedLock::SetRetry(const int count, const int delay) {
		m_retryCount = count;
		m_retryDelay = delay;
	}

	// ----------------
	// lock the resource
	// ----------------
	bool CRedLock::Lock(const char *resource, const int ttl, CLock &lock) {
		sds val = GetUniqueLockId();
		if (!val) {
			return false;
		}
		lock.m_resource = sdsnew(resource);
		lock.m_val = val;
#if 0
		printf("Get the unique id is %s\n", val);
#else
		//LOG_WARN << __FUNCTION__ << " Get the unique id is " << val;
#endif
		int retryCount = m_retryCount;
		do {
			int n = 0;
			int startTime = (int)time(NULL) * 1000;
			int slen = (int)m_redisServer.size();
			for (int i = 0; i < slen; i++) {
				if (LockInstance(m_redisServer[i], resource, val, ttl)) {
					n++;
				}
			}
			//Add 2 milliseconds to the drift to account for Redis expires
			//precision, which is 1 millisecond, plus 1 millisecond min drift
			//for small TTLs.
			int drift = (ttl * m_clockDriftFactor) + 2;
			int validityTime = ttl - ((int)time(NULL) * 1000 - startTime) - drift;
#if 0
			printf("The resource validty time is %d, n is %d, quo is %d\n",
				validityTime, n, m_quoRum);
#else
			//LOG_WARN << __FUNCTION__ << " The resource validty time is " << validityTime << ", n is " << n << ", quo is " << m_quoRum;
#endif
			if (n >= m_quoRum && validityTime > 0) {
				lock.m_validityTime = validityTime;
				return true;
			}
			else {
				Unlock(lock);
			}
			// Wait a random delay before to retry
			int delay = rand() % m_retryDelay + floor(m_retryDelay / 2);
			usleep(delay * 1000);
			retryCount--;
		} while (retryCount > 0);
		return false;
	}

	// ----------------
	// release resource
	// ----------------
	bool CRedLock::ContinueLock(const char *resource, const int ttl, CLock &lock) {
		sds val = GetUniqueLockId();
		if (!val) {
			return false;
		}
		lock.m_resource = sdsnew(resource);
		lock.m_val = val;
		if (m_continueLock.m_resource == NULL) {
			m_continueLock.m_resource = sdsnew(resource);
			m_continueLock.m_val = sdsnew(val);
		}
#if 0
		printf("Get the unique id is %s\n", val);
#else
		//LOG_WARN << __FUNCTION__ << " Get the unique id is " << val;
#endif
		int retryCount = m_retryCount;
		do {
			int n = 0;
			int startTime = (int)time(NULL) * 1000;
			int slen = (int)m_redisServer.size();
			for (int i = 0; i < slen; i++) {
				if (ContinueLockInstance(m_redisServer[i], resource, val, ttl)) {
					n++;
				}
			}
			// update old val
			sdsfree(m_continueLock.m_val);
			m_continueLock.m_val = sdsnew(val);
			//Add 2 milliseconds to the drift to account for Redis expires
			//precision, which is 1 millisecond, plus 1 millisecond min drift
			//for small TTLs.
			int drift = (ttl * m_clockDriftFactor) + 2;
			int validityTime = ttl - ((int)time(NULL) * 1000 - startTime) - drift;
#if 0
			printf("The resource validty time is %d, n is %d, quo is %d\n",
				validityTime, n, m_quoRum);
#else
			//LOG_WARN << __FUNCTION__ << " The resource validty time is " << validityTime << ", n is " << n << ", quo is " << m_quoRum;
#endif
			if (n >= m_quoRum && validityTime > 0) {
				lock.m_validityTime = validityTime;
				return true;
			}
			else {
				Unlock(lock);
			}
			// Wait a random delay before to retry
			int delay = rand() % m_retryDelay + floor(m_retryDelay / 2);
			usleep(delay * 1000);
			retryCount--;
		} while (retryCount > 0);
		return false;
	}

	// ----------------
	// lock the resource
	// ----------------
	bool CRedLock::Unlock(const CLock &lock) {
		int slen = (int)m_redisServer.size();
		for (int i = 0; i < slen; i++) {
			UnlockInstance(m_redisServer[i], lock.m_resource, lock.m_val);
		}
		return true;
	}

	// ----------------
	// lock the resource milliseconds
	// ----------------
	bool CRedLock::LockInstance(redisContext *c, const char *resource,
		const char *val, const int ttl) {
		redisReply *reply;
		reply = (redisReply *)redisCommand(c, "set %s %s px %d nx",
			resource, val, ttl);
		if (reply) {
#if 0
			printf("Set return: %s [null == fail, OK == success]\n", reply->str);
#else
			//LOG_WARN << __FUNCTION__ << " Set return: " << reply->str << " [null == fail, OK == success]";
#endif
		}
		if (reply && reply->str && strcmp(reply->str, "OK") == 0) {
			freeReplyObject(reply);
			return true;
		}
		if (reply) {
			freeReplyObject(reply);
		}
		return false;
	}

	// ----------------
	// 对redismaster续锁, 私有函数
	// ----------------
	bool CRedLock::ContinueLockInstance(redisContext *c, const char *resource,
		const char *val, const int ttl) {
		int argc = 7;
		sds sdsTTL = sdsempty();
		sdsTTL = sdscatprintf(sdsTTL, "%d", ttl);
		char *continueLockScriptArgv[] = { (char*)"EVAL",
										  m_continueLockScript,
										  (char*)"1",
										  (char*)resource,
										  m_continueLock.m_val,
										  (char*)val,
										  sdsTTL };
		redisReply *reply = RedisCommandArgv(c, argc, continueLockScriptArgv);
		sdsfree(sdsTTL);
		if (reply) {
#if 0
			printf("Set return: %s [null == fail, OK == success]\n", reply->str);
#else
			//LOG_WARN << __FUNCTION__ << " Set return: " << reply->str << " [null == fail, OK == success]";
#endif
		}
		if (reply && reply->str && strcmp(reply->str, "OK") == 0) {
			freeReplyObject(reply);
			return true;
		}
		if (reply) {
			freeReplyObject(reply);
		}
		return false;
	}

	// ----------------
	// 对redismaster解锁
	// ----------------
	void CRedLock::UnlockInstance(redisContext *c, const char *resource,
		const char *val) {
		int argc = 5;
		char *unlockScriptArgv[] = { (char*)"EVAL",
									m_unlockScript,
									(char*)"1",
									(char*)resource,
									(char*)val };
		redisReply *reply = RedisCommandArgv(c, argc, unlockScriptArgv);
		if (reply) {
			freeReplyObject(reply);
		}
	}

	// ----------------
	// 对redismaster执行脚本命令
	// ----------------
	redisReply * CRedLock::RedisCommandArgv(redisContext *c, int argc, char **inargv) {
		char **argv;
		argv = convertToSds(argc, inargv);
		/* Setup argument length */
		size_t *argvlen;
		argvlen = (size_t *)malloc(argc * sizeof(size_t));
		for (int j = 0; j < argc; j++)
			argvlen[j] = sdslen(argv[j]);
		redisReply *reply = NULL;
		reply = (redisReply *)redisCommandArgv(c, argc, (const char **)argv, argvlen);
		if (reply) {
#if 0
			printf("RedisCommandArgv return: %lld\n", reply->integer);
#else
			//LOG_WARN << __FUNCTION__ << " RedisCommandArgv return: " << reply->integer;
#endif
		}
		free(argvlen);
		sdsfreesplitres(argv, argc);
		return reply;
	}

	// ----------------
	// 获取唯一id
	// ----------------
	sds CRedLock::GetUniqueLockId() {
		unsigned char buffer[20];
		if (read(m_fd, buffer, sizeof(buffer)) == sizeof(buffer)) {
			//获取20byte的随机数据
			sds s;
			s = sdsempty();
			for (int i = 0; i < 20; i++) {
				s = sdscatprintf(s, "%02X", buffer[i]);
			}
			return s;
		}
		else {
			//读取失败
#if 0
			printf("Error: GetUniqueLockId %d\n", __LINE__);
#else
			//LOG_WARN << __FUNCTION__ << " Error: GetUniqueLockId " << __LINE__;
#endif
		}
		return NULL;
	}

	__thread RedisLock::CRedLock* ThreadLocalSingleton::s_value_ = 0;
	typename ThreadLocalSingleton::Deleter ThreadLocalSingleton::s_deleter_;
}