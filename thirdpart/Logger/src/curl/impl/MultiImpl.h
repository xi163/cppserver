#pragma once

#include "../../Macro.h"
#include "../../op/Operation.h"
#include "curl/curl.h"

namespace Curl {

	class ClientImpl;
	class MultiImpl {
		friend class ClientImpl;
	public:
		MultiImpl();
		~MultiImpl();
	protected:
		int perform();
		inline bool validate() { return NULL != curlm_; }
	protected:
		int add_handle(CURL* curl);
		int select();
		int info_read();
		int remove_handle(CURL* curl);
	private:
		CURLM* curlm_;
		std::list<CURL*> list_curl_;
	private:
		int add_handles();
		int remove_handles();
	};
}