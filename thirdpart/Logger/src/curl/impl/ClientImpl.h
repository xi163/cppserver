#pragma once

#include "../../Macro.h"

#pragma comment(lib, "ws2_32.lib")

#include "EasyImpl.h"
#include "MultiImpl.h"

#define CHECKPTR_BREAK(x) \
		if (NULL == (x)) { \
			break;  \
		}

#define CHECKCURLE_BREAK(x) \
		if (CURLE_OK != (x)) { \
			break;  \
		}

#define CHECKCURLM_BREAK(x) \
		if (CURLM_OK != (x)) { \
			break;  \
		}

namespace Curl {

	class EasyImpl;
	class MultiImpl;
	class ClientImpl {
	public:
		ClientImpl();
		ClientImpl(bool sync);
		~ClientImpl();
		int check(char const* url, double& size);
		int get(
			char const* url,
			std::list<std::string> const* headers,
			std::string* resp = NULL,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int post(
			char const* url,
			std::list<std::string> const* headers,
			char const* spost,
			std::string* resp,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int upload(
			char const* url,
			std::list<Operation::Args> const* args,
			std::string* resp,
			OnProgress onProgress,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int download(
			char const* url,
			char const* savepath,
			OnBuffer onBuffer,
			OnProgress onProgress,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int perform();
	private:
		template <class T> static inline T* New() {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T();
		}
		template <class T> static inline void Delete(T* ptr) {
			ptr->~T();
			free(ptr);
		}
	private:
		MultiImpl* multi_;
		std::list<EasyImpl*> list_easy_;
	};
}