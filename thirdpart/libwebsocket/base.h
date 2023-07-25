#ifndef _MUDUO_BASE_OBJECT_H_
#define _MUDUO_BASE_OBJECT_H_

namespace muduo {
	namespace base {
		
		class copyable {
		protected:
			copyable() = default;
			~copyable() = default;
		};

		class noncopyable {
		public:
			noncopyable(const noncopyable&) = delete;
			void operator=(const noncopyable&) = delete;
		protected:
			noncopyable() = default;
			~noncopyable() = default;
		};

	}//namespace base
}//namespace muduo

#endif