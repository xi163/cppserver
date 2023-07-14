#include "../Client.h"
#include "ClientImpl.h"
#include "../../auth/auth.h"

namespace Curl {

	Client::Client()
		: impl_(New<ClientImpl>()) {
		//placement new
	}

	Client::Client(bool sync)
		: impl_(New<ClientImpl>(sync)) {
		//placement new
	}

	Client::~Client() {
		Delete<ClientImpl>(impl_);
	}

	int Client::check(char const* url, double& size) {
		AUTHORIZATION_CHECK_R;
		return impl_->check(url, size);
	}

	int Client::get(
		char const* url,
		std::list<std::string> const* headers,
		std::string* resp,
		char const* spath,
		bool dump, FILE* fd) {
		AUTHORIZATION_CHECK_R;
		return impl_->get(url, headers, resp, spath, dump, fd);
	}

	int Client::post(
		char const* url,
		std::list<std::string> const* headers,
		char const* spost,
		std::string* resp,
		char const* spath,
		bool dump, FILE* fd) {
		AUTHORIZATION_CHECK_R;
		return impl_->post(url, headers, spost, resp, spath, dump, fd);
	}

	int Client::upload(
		char const* url,
		std::list<Operation::Args> const* args,
		std::string* resp,
		OnProgress onProgress,
		char const* spath,
		bool dump, FILE* fd) {
		AUTHORIZATION_CHECK_R;
		return impl_->upload(url, args, resp, onProgress, spath, dump, fd);
	}

	int Client::download(
		char const* url,
		char const* savepath,
		OnBuffer onBuffer,
		OnProgress onProgress,
		char const* spath,
		bool dump, FILE* fd) {
		AUTHORIZATION_CHECK_R;
		return impl_->download(url, savepath, onBuffer, onProgress, spath, dump, fd);
	}
}