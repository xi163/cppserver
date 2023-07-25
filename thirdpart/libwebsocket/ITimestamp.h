#ifndef _MUDUO_ITIMESTAMP_H_
#define _MUDUO_ITIMESTAMP_H_

#include "Logger/src/utils/utils.h"

#include <libwebsocket/base.h>

namespace muduo {

	class ITimestamp {
	public:
		virtual void swapPtr(ITimestamp* that) = 0;
		virtual std::string toString() const = 0;
		virtual std::string toFormattedString(bool showMicroseconds = true) const = 0;
		virtual bool valid() const = 0;
		virtual int64_t microSecondsSinceEpoch() const = 0;
		virtual time_t secondsSinceEpoch() const = 0;
	};

}//namespace muduo

#endif