#include "MultiImpl.h"
#include "ClientImpl.h"

namespace Curl {

	MultiImpl::MultiImpl() {
		//init multi
		curlm_ = ::curl_multi_init();
	}

	int MultiImpl::add_handle(CURL* curl) {
		int rc = 0;
		do {
			CHECKPTR_BREAK(curl);
			CHECKPTR_BREAK(curlm_);

#if 0
			if (CURLM_OK != ::curl_multi_add_handle(curlm_, curl)) {
				rc = -1;
				break;
			}
#endif
			list_curl_.push_back(curl);

		} while (0);
		return rc;
	}

	int MultiImpl::add_handles() {
		int rc = 0;
		for (std::list<CURL*>::iterator it = list_curl_.begin();
			it != list_curl_.end();
			++it) {
			//easy 加入到 multi, 异步支持
			if (CURLM_OK != ::curl_multi_add_handle(curlm_, *it)) {
				rc = -1;
				break;
			}
		}
		if (-1 == rc) {
			for (std::list<CURL*>::iterator it = list_curl_.begin();
				it != list_curl_.end();
				++it) {
				::curl_multi_remove_handle(curlm_, *it);
			}
		}
		return rc;
	}

	int MultiImpl::remove_handle(CURL* curl) {
		int rc = 0;
		do {
			CHECKPTR_BREAK(curl);
			CHECKPTR_BREAK(curlm_);
#if 0
			if (CURLM_OK != ::curl_multi_remove_handle(curlm_, curl)) {
				rc = -1;
				break;
			}
#endif
			list_curl_.remove(curl);

		} while (0);
		return rc;
	}

	int MultiImpl::remove_handles() {
		if (curlm_) {
			for (std::list<CURL*>::iterator it = list_curl_.begin();
				it != list_curl_.end();
				) {
				::curl_multi_remove_handle(curlm_, *it);
				it = list_curl_.erase(it);
			}
			list_curl_.clear();
		}
		return 0;
	}

	int MultiImpl::select() {
		REQState rc = eFailed;
		do {
			CHECKPTR_BREAK(curlm_);

			timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			CURLMcode multicode;
			long curl_timeo = -1;

			multicode = ::curl_multi_timeout(curlm_, &curl_timeo);
			if (CURLM_OK == multicode && curl_timeo >= 0) {
				timeout.tv_sec = curl_timeo / 1000;
				if (timeout.tv_sec > 1)
					timeout.tv_sec = 1;
				else
					timeout.tv_usec = (curl_timeo % 1000) * 1000;
			}

			int maxfd = -1;
			fd_set readfds, writefds, exceptfds;
			do {
				FD_ZERO(&readfds);
				FD_ZERO(&writefds);
				FD_ZERO(&exceptfds);

				multicode = ::curl_multi_fdset(curlm_, &readfds, &writefds, &exceptfds, &maxfd);
				CHECKCURLM_BREAK(multicode);

				if (-1 != maxfd) {
					break;
				}

				int running = 0;
				while (CURLM_CALL_MULTI_PERFORM == ::curl_multi_perform(curlm_, &running)) {
					assert(false);
				}
			} while (-1 == maxfd);

			int err = ::select(maxfd + 1, &readfds, &writefds, &exceptfds, &timeout);
			switch (err)
			{
			case -1:
				/* select error */
				break;
			case 0:
				/* select timeout */
				rc = eTimeout;
				break;
			default:
				/* readable/writable sockets */
				rc = eContinue;
				break;
			}
		} while (0);
		return (eFailed == rc || eInterrupt == rc || eNetError == rc) ? -1 : 0;
	}

	int MultiImpl::perform() {
		int rc = -1;
		do {
			CHECKPTR_BREAK(curlm_);

			//easy 加入到 multi, 异步支持
			if (0 != (rc = add_handles())) {
				break;
			}

			//run curl_multi_perform
			int running = 0;
			while (CURLM_CALL_MULTI_PERFORM == ::curl_multi_perform(curlm_, &running)) {
				assert(false);
			}

			//if (pFMop)
			//	pFMop->Open();

			do {
				if (0 != (rc = this->select())) {
					break;
				}

				while (CURLM_CALL_MULTI_PERFORM == ::curl_multi_perform(curlm_, &running)) {
					assert(false);
				}
			} while (running);

			//if (pFMop)
			//	pFMop->Close();

			info_read();

			//移除 multi 中 easy
			remove_handles();

			rc = 0;

		} while (0);

		return rc;
	}

	int MultiImpl::info_read() {
		int rc = 0;
		int left;
		CURLMsg* msg;
		while ((msg = ::curl_multi_info_read(curlm_, &left))) { //获取当前解析的curl的相关传输信息
			if (CURLMSG_DONE == msg->msg) {
				if (CURLE_OK != msg->data.result) {
					rc = -1;
				}
			}
			else {
				rc = -1;
			}
		}
		return rc;
	}

	MultiImpl::~MultiImpl() {
		if (curlm_) {
			for (std::list<CURL*>::iterator it = list_curl_.begin();
				it != list_curl_.end();
				) {
				::curl_multi_remove_handle(curlm_, *it);
				it = list_curl_.erase(it);
			}
			list_curl_.clear();
			::curl_multi_cleanup(curlm_);
			curlm_ = NULL;
		}
	}
}