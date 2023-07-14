#pragma once

#include "Args.h"
#include "../op/Operation.h"

namespace Curl {

	typedef std::function<size_t(Operation::COperation* obj, void* buffer, size_t size, size_t nmemb)> OnBuffer;
	typedef std::function<void(Operation::COperation* obj, double ltotal, double lnow)> OnProgress;

	class ClientImpl;
	class Client {
	public:
		Client();
		Client(bool sync);
		~Client();
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
	private:
		template <class T> static inline T* New() {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T();
		}
		template <class T> static inline T* New(bool sync) {
			void* ptr = (void*)malloc(sizeof(T));
			return new(ptr) T(sync);
		}
		template <class T> static inline void Delete(T* ptr) {
			ptr->~T();
			free(ptr);
		}
	private:
		ClientImpl* impl_;
	};
}