#include "ClientImpl.h"
#include "../../op/mem/impl/MemImpl.h"
#include "../../op/file/impl/FileImpl.h"

//multipart/form-data详细介绍 http://blog.csdn.net/yankai0219/article/details/8159701
//C++ curl库 源码编译及使用（VS2019）https://blog.csdn.net/xray2/article/details/120496410

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

	ClientImpl::ClientImpl()
		: multi_(NULL) {
	}

	ClientImpl::ClientImpl(bool sync)
		: multi_(sync ? (New<MultiImpl>()) : NULL) {
		//placement new
	}

	int ClientImpl::check(char const* url, double& size) {
		EasyImpl easy;
		return easy.check(url, size);
	}

	int ClientImpl::get(
		char const* url,
		std::list<std::string> const* headers,
		std::string* resp,
		char const* spath,
		bool dump, FILE* fd) {
		int rc = -1;
		do {
			EasyImpl easy;
			//上传模式, 则写内存
			Operation::MemImpl op;
			easy.SetOperation(&op);

			if (0 != easy.buildGet(url, headers, spath, dump, fd)) {
				break;
			}
			easy.Open();
			if (0 != easy.perform())
				break;
			if (NULL != resp) {
				easy.GetBuffer(*resp);
			}
			easy.Close();
			rc = 0;
		} while (0);
		return rc;
	}

	int ClientImpl::post(
		char const* url,
		std::list<std::string> const* headers,
		char const* spost,
		std::string* resp,
		char const* spath,
		bool dump, FILE* fd) {
		int rc = -1;
		do {
			EasyImpl easy;
			//上传模式, 则写内存
			Operation::MemImpl op;
			easy.SetOperation(&op);

			if (0 != easy.buildPost(url, headers, spost, spath, dump, fd)) {
				break;
			}
			if (0 != easy.perform())
				break;
			if (NULL != resp) {
				easy.GetBuffer(*resp);
			}
			rc = 0;
		} while (0);
		return rc;
	}

	int ClientImpl::upload(
		char const* url,
		std::list<Operation::Args> const* args,
		std::string* resp,
		OnProgress onProgress,
		char const* spath,
		bool dump, FILE* fd) {
		int rc = -1;
		do {
			EasyImpl easy;
			//上传模式, 则写内存
			Operation::MemImpl op;
			easy.SetOperation(&op);

			if (0 != easy.buildUpload(url, args, onProgress, spath, dump, fd)) {
				break;
			}
			if (!multi_) {
				easy.Open();
				if (0 != easy.perform())
					break;
				if (NULL != resp) {
					easy.GetBuffer(*resp);
				}
				easy.Close();
			}
			else {
				if (0 != multi_->add_handle(easy.curl_)) {
					break;
				}
				easy.Open();
				if (0 != multi_->perform()) {
					easy.Close();
					break;
				}
				if (NULL != resp) {
					easy.GetBuffer(*resp);
				}
				easy.Close();
			}
			rc = 0;
		} while (0);
		return rc;
	}

#if 0
	int ClientImpl::addUpload(
		char const* url,
		std::list<IOperation::Args> const* args,
		std::string* resp,
		OnBuffer onBuffer,
		OnProgress onProgress,
		char const* spath,
		bool dump, FILE* fd) {
		int rc = -1;
		do
		{
			EasyImpl* easy = new EasyImpl();
			CHECKPTR_BREAK(easy);

			if (0 != easy->buildUpload(url, args, callback, spath, dump, fd)) {
				delete easy;
				break;
			}
			this->list_easy_.push_back(easy);
		} while (0);

		return rc;
	}
#endif

	int ClientImpl::download(
		char const* url,
		char const* savepath,
		OnBuffer onBuffer,
		OnProgress onProgress,
		char const* spath,
		bool dump, FILE* fd) {
		int rc = -1;
		do {
			EasyImpl easy;
			//下载模式, 则写文件
			Operation::FileImpl op(savepath);
			easy.SetOperation(&op);

			if (0 != easy.buildDownload(url, onBuffer, onProgress, spath, dump, fd)) {
				break;
			}
			if (!multi_) {
				if (!easy.Open(Operation::Mode::M_WRITE)) {
					break;
				}
				if (0 != easy.perform()) {
					easy.Close();
					break;
				}
				easy.Close();
			}
			else {
				if (0 != multi_->add_handle(easy.curl_)) {
					break;
				}
				if (!easy.Open(Operation::Mode::M_WRITE)) {
					break;
				}
				if (0 != multi_->perform()) {
					easy.Close();
					break;
				}
				easy.Close();
			}
			rc = 0;
		} while (0);
		return rc;
	}

#if 0
	int ClientImpl::addDownload(
		char const* url,
		callback_t callback,
		char const* spath,
		bool dump, FILE* fd) {
		do {
			EasyImpl* easy = new EasyImpl();
			if (0 != easy->buildDownload(url, callback, spath, dump, fd)) {
				delete easy;
				break;
			}
			this->list_easy_.push_back(easy);
		} while (0);

		return 0;
	}
#endif

	int ClientImpl::perform() {
		return 0;
	}

	ClientImpl::~ClientImpl() {
		if (multi_) {
			Delete<MultiImpl>(multi_);
			multi_ = NULL;
		}
	}
}

#if 0

#include <stdio.h>  
#include <stdlib.h>

#include "httpCurl/MemOperation.h"
#include "httpCurl/FileOperation.h"
#include "httpCurl/ClientImpl.h"
#include "httpCurl/EasyImpl.h"

unsigned lasttime = utils::_now_ms();

void onUpload(Curl::EasyImpl* easy, double ftotal, double fnow) {
	printf("progress[%.3f / %.3f] [%d%%]\n", fnow, ftotal, (int)((fnow / ftotal) * 100));
	if (fnow != 0 && fnow == ftotal)
	{
		lasttime = utils::_now_ms();
	}
}

//测试上传
void testUpload()
{
	Curl::ClientImpl req;
	Curl::ClientImpl reqm(true);

	std::list<Args> slist;
	IOperation::Args args;
// 	args.strkey = "file1";
// 	Operation::FileImpl file1("C:/Users/Administrator/Desktop/abc.png"); //待上传文件
// 	args.value = &file1;
// 	sprintf(args.fileinfo.szfilename, "*.png");
// 	slist.push_back(args);

	std::string resp;
#if 0
// 	req.upload("http://192.168.1.80:8089/cgi-bin/upload",
// 		&slist, &resp, NULL, NULL, true);
#else
// 	reqm.upload("http://192.168.1.80:8089/cgi-bin/upload",
// 		&slist, &resp, NULL, NULL, true);
#endif
//	printf("返回数据1:\n%s\n", resp.c_str());

// 	args.strkey = "file2";
// 	Operation::FileImpl file2("E:/svn/setup/acl-v3.1.3.zip"); //待上传文件
// 	args.value = &file2;
// 	sprintf(args.fileinfo.szfilename, "*.zip");
// 	slist.push_back(args);

#if 0
// 	req.upload("http://192.168.1.80:8089/cgi-bin/upload",
// 		&slist, &resp, NULL, NULL, true);
#else
// 	reqm.upload("http://192.168.1.80:8089/cgi-bin/upload",
// 		&slist, &resp, NULL, NULL, true);
#endif

	args.key = "file1";
	Operation::FileImpl file1("F:/Office Professional Plus 2007_cn.7z"); // 待上传文件
	args.value = &file1;
	sprintf(args.fi.filename, "*.rar");
	slist.push_back(args);

#if 1
	req.upload("http://192.168.1.80:8081/upload.cgi",
		&slist, &resp, onUpload, NULL, false);
#else
	req.upload("http://192.168.1.80:8088/cgi-bin/upload2",
		&slist, &resp, onUpload, NULL, false);
#endif
	unsigned tickcount = utils::_now_ms();
	printf("返回数据2:\n%s\n", resp.c_str());

	printf("time = %d ms", tickcount - lasttime);
};

void onDownload(Curl::EasyImpl* easy, double ftotal, double fnow) {
	Operation::FileImpl* f = (Operation::FileImpl*)easy->GetOperation();
	std::string path = f->Path();
	int pos;
	pos = path.find_last_of('\\');
	std::string filename = path.substr(pos + 1, -1);
	printf("file[%s] progress[%.3f / %.3f] [%d%%]\n", filename.c_str(), fnow, ftotal, (int)((fnow / ftotal) * 100));
}

//测试下载
void testDownload()
{
	Curl::ClientImpl req;
	Curl::ClientImpl reqm(true);

#if 0
	req.download("http://192.168.1.80:8089/cgi-bin/download?fileid=group1/M00/00/77/wKgBUFc-cEKEV8plAAAAAAj6Uz0475.png&range=0-100000000",
		"F:/download1.png",
		NULL, NULL, false);
	req.download("http://192.168.1.80:8089/cgi-bin/download?fileid=group1/M00/00/78/wKgBUFdCfa2EPjJlAAAAADCKH5k221.zip&range=0-100000000",
		"F:/download2.zip",
		NULL, NULL, false);
#else
// 	reqm.download("http://192.168.1.80:8089/cgi-bin/download?fileid=group1/M00/00/77/wKgBUFc-cEKEV8plAAAAAAj6Uz0475.png&range=0-100000000",
// 		"F:/download1.png",
// 		NULL, NULL, false);
// 	reqm.download("http://192.168.1.80:8089/cgi-bin/download?fileid=group1/M00/00/78/wKgBUFdCfa2EPjJlAAAAADCKH5k221.zip&range=0-100000000",
// 		"F:/download2.zip",
// 		NULL, NULL, false);
// 	reqm.download("http://qcyn.sinaimg.cn/2013/0614/U9113P1032DT20130614105804.jpg",
// 		"F:/download2.jpg",
// 		NULL, NULL, false);
// 	FILE* pf = fopen("f:/2.txt", "w+");
// 	reqm.download("http://m5.file.xiami.com/712/55712/911528955/1773565730_15832377_l.mp3?auth_key=d93295ad362fc35e52b41c0b69fac67a-1475856000-0-null",
// 		"F:/download2.mp3",
// 		onDownload, NULL, true, pf);
// 	fflush(pf);
// 	fclose(pf);

	reqm.download("http://192.168.1.80:8889/download/mime_----WebKitFormBoundary8fvE4QcsHbtmA2uB_0000000000000000000026522036_24.m4a",
		"F:/0dfasfdsfdsfdsfdsf.m4a",
		onDownload, NULL, false, NULL);
#endif
}

void testGet()
{
	Curl::ClientImpl req;
	Curl::ClientImpl reqm(true);

	FILE* pf = fopen("e:/1.txt", "w+");
	std::string resp;
	reqm.get("http://192.168.1.80:8088/get_rec.cgi?dc=1272&debug=0",
		NULL,
		&resp,
		NULL, true, pf);
	fflush(pf);
	fclose(pf);
	printf("%s\n", resp.c_str());

}

void testPost()
{
	char const* url = "http://test.api.hcicloud.com:8880/nlu/recognize";

	char const* body = " {\"question\":{\"query\": \"北京今天的天气怎么样啊\"},\"context\": {}} ";

	char utf8[256] = { 0 };
	GBKToUTF8(body, strlen(body), utf8, sizeof(utf8));

	std::list<std::string> header;
//	header.push_back("Content-Type: application/json;charset=utf-8"); //application/json;charset=gb2312
	header.push_back("x-session-key:eccbdc039d5f07cec29cd9575d4cab71");
	header.push_back("x-udid:101:123456789");
	header.push_back("x-result-format:json");
	header.push_back("x-app-key:285d5470");
	header.push_back("x-sdk-version:5.0");
	header.push_back("x-request-date:2016-07-14 11:38:07");
	header.push_back("x-task-config:capkey=nlu.cloud,intention=weather");

	std::string resp;
	Curl::ClientImpl req;

	req.post(url, &header, utf8, &resp, NULL, true);

	char gbk[512] = { 0 };
	UTF8ToGBK(resp.c_str(), resp.length(), gbk, sizeof(gbk));
	printf("\n%s\n", gbk);
}

int main(int argc, char* argv[])
{
	testGet();
	testPost();
	testUpload();
	testDownload();
	return 0;
}

#endif