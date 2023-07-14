#pragma once

#include "../../Macro.h"
#include "../../op/Operation.h"
#include "../Args.h"
#include "curl/curl.h"

namespace Curl {

	class ClientImpl;
	class EasyImpl;

	typedef std::function<size_t(Operation::COperation* obj, void* buffer, size_t size, size_t nmemb)> OnBuffer;
	typedef std::function<void(Operation::COperation* obj, double ltotal, double lnow)> OnProgress;

	class EasyImpl : public Operation::COperation {
		friend class ClientImpl;
		enum { EUpload, EDownload };
	public:
		EasyImpl();
		~EasyImpl();
	protected:
		int buildGet(
			char const* url,
			std::list<std::string> const* headers,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int buildPost(
			char const* url,
			std::list<std::string> const* headers,
			char const* spost,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int buildUpload(
			char const* url,
			std::list<Operation::Args> const* args,
			OnProgress onProgress,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int buildDownload(
			char const* url,
			OnBuffer onBuffer,
			OnProgress onProgress,
			char const* spath = NULL,
			bool dump = true, FILE* fd = stderr);
		int perform();

	protected:
		int check(char const* url, double& size);
		int setTimeout();
		int setDebug(bool dump = true, FILE* fd = stderr);
		int setUrl(char const* url);
		int addHeader(std::list<std::string> const* headers);
		int addPost(std::list<Operation::Args> const* args, char const* spost);
		int setProxy(char const* sproxy = NULL, char const* sagent = NULL);
		int setSSLCA(char const* spath = NULL);
		int setCallback(void* readcbk = NULL, void* writecbk = NULL, void* progresscbk = NULL);
	protected:
		CURL* curl_;
		curl_httppost* formpost_;
		curl_httppost* lastptr_;
		curl_slist* headerlist_;
		unsigned long lasttime_;
		OnProgress progress_cb_;
		OnBuffer buffer_cb_;
		int mode_;
		bool finished_;
	private:
		struct debug_data_t {
			bool dump_flag_;
			FILE* fd;
		};
		debug_data_t debug_data_;
	protected:
		size_t readCallback(void* buffer, size_t size, size_t nmemb);
		size_t writeCallback(void* buffer, size_t size, size_t nmemb);
		int progressCallback(double dltotal, double dlnow, double ultotal, double ulnow);
	private:
		static int debugCallback_(CURL* curl, curl_infotype type, char* data, size_t size, void* userp);
		static void dump_(char const* text, FILE* stream, unsigned char* ptr, size_t size);
		static size_t readCallback_(void* buffer, size_t size, size_t nmemb, void* stream);
		static size_t writeCallback_(void* buffer, size_t size, size_t nmemb, void* stream);
		static int progressCallback_(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
	private:
		bool formAdd(CURL* curl, Operation::Args const& args);
		bool formAdd(CURL* curl, std::list<Operation::Args> const& args);
		void purge();
	};
}