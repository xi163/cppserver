#ifndef INCLUDE_INC_H
#define INCLUDE_INC_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>// memset
#include <assert.h>

//被 *.c 文件包含会有问题
#include <algorithm>
#include <utility>
#include <random>
#include <chrono>

#include <atomic> //atomic_llong
#include <memory>
#include <set>
#include <unordered_set>
#include <map>
#include <list>
#include <vector>
#include <deque>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <functional>
#include <ios>

#include <math.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if.h>
//#include <net/if.h>

//#include <curl/curl.h>

#include <boost/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/singleton.hpp>
#include <boost/date_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp> //boost::iequals
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/regex.hpp>
#include <boost/locale.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/get_pointer.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <muduo/base/Logging.h>
//#include <muduo/base/AsyncLogging.h>
#include <muduo/base/noncopyable.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/base/TimeZone.h>
#include <muduo/base/Atomic.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TimerId.h>
#include <muduo/net/Reactor.h>
#include <muduo/net/protorpc/RpcServer.h>
#include <muduo/net/protorpc/RpcChannel.h>
#include <muduo/net/http/HttpContext.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <muduo/net/libwebsocket/context.h>
#include <muduo/net/libwebsocket/server.h>
#include <muduo/net/libwebsocket/ssl.h>

#include <google/protobuf/message.h>

using boost::serialization::singleton;
using namespace boost::posix_time;
using namespace boost::gregorian;

#include "Logger.h"

#include "Env.h"
#include "StdRandom.h"
#include "SubNetIP.h"
#include "NetCardIP.h"
#include "Utils.h"
#include "IPFinder.h"

// #include "codec/aes.h"
// #include "codec/mymd5.h"
// #include "codec/base64.h"
// #include "codec/htmlcodec.h"
// #include "codec/urlcodec.h"

#include "zookeeperclient/zookeeperclient.h"
#include "zookeeperclient/zookeeperlocker.h"
#include "MongoDB/MongoDBClient.h"
#include "RedisClient/RedisClient.h"
#include "RedisLock/redlock.h"
#include "TraceMsg/TraceMsg.h"

#define likely(x)                   __builtin_expect(!!(x), 1)
#define unlikely(x)                 __builtin_expect(!!(x), 0)

#define READ_LOCK(mutex)            boost::shared_lock<boost::shared_mutex> guard(mutex)
#define WRITE_LOCK(mutex)           boost::lock_guard<boost::shared_mutex> guard(mutex)

#define CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)
#define CALLBACK_4(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, ##__VA_ARGS__)
#define CALLBACK_5(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, ##__VA_ARGS__)

#define REDISCLIENT muduo::ThreadLocalSingleton<RedisClient>::instance()
#define MONGODBCLIENT MongoDBClient::ThreadLocalSingleton::instance()
#define REDISLOCK RedisLock::ThreadLocalSingleton::instance()

#endif