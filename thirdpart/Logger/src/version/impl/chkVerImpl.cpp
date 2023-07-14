#include "chkVerImpl.h"
#include "../../utils/impl/utilsImpl.h"
#include "../../utils/impl/regedt32.h"
#include "../../ini/ini.h"
#include "../../ini/impl/iniImpl.h"

#include "../../log/impl/LoggerImpl.h"
#include "../../excp/impl/excpImpl.h"

#include "../../curl/impl/ClientImpl.h"
//#include "../../op/mem/impl/MemImpl.h"
#include "../../op/file/impl/FileImpl.h"

#include "../../crypt/mymd5.h"

namespace utils {

	//-1失败，退出 0成功，退出 1失败，继续
	static inline void _updateVersion(INI::Section& version, std::string const& dir, std::function<void(int rc)> cb) {
		__MY_TRY();
		std::string url = version["download"];
		std::string::size_type pos = url.find_last_of('/');
		std::string filename = url.substr(pos + 1, -1);
		if (!utils::_mkDir(dir.c_str())) {
			__PLOG_ERROR("创建下载目录失败..%s\n\t1.可能权限不够，请选择其它盘重新安装，不要安装在C盘，或以管理员身份重新启动!\n\t2.不要安装在中文文件夹", dir.c_str());
			__LOG_CONSOLE_CLOSE(10000, true);
			cb(-1);//失败，退出
			return;
		}
		std::string path = dir;
		//std::string path = utils::_GetModulePath() + "/download";
		path += "/" + filename;
		Operation::FileImpl f(path.c_str());
		if (f.Valid()) {
			std::vector<char> data;
			f.Buffer(data);
			f.Close();
			__PLOG_DEBUG("安装包已存在! 共 %d 字节，准备校验...", data.size());
			if (data.size() > 0) {
				char md5[32 + 1] = { 0 };
				MD5Encode32(&data.front(), data.size(), md5, 0);
				if (atol(version["size"].c_str()) == data.size() &&
					strncasecmp(md5, version["md5"].c_str(), strlen(md5)) == 0) {
					__PLOG_DEBUG("校验成功，开始安装新版程序包...");
					std::string content = version["context"];
					utils::_replaceEscChar(content);
					__PLOG_WARN("*******************************************");
					//PLOG_WARN(content.c_str(), m["no"].c_str());
					__PLOG_WARN(content.c_str());
					__PLOG_WARN("*******************************************");
					//::WinExec(path.c_str(), SW_SHOWMAXIMIZED);
					::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
					__LOG_CONSOLE_CLOSE(5000);
					cb(0);//成功，退出
					return;
				}
			}
			__TLOG_DEBUG("校验失败，重新下载安装包... %s", url.c_str());
		}
		else {
			__TLOG_DEBUG("开始下载安装包... %s", url.c_str());
		}
		std::vector<char> data;
		Curl::ClientImpl req(true);
		if (req.download(
			url.c_str(),
			path.c_str(),
			[&](Operation::COperation* obj, void* buffer, size_t size, size_t nmemb) -> size_t {
				size_t n = obj->Write(buffer, size, nmemb);
				if (n > 0) {
					int offset = data.size();
					data.resize(offset + n);
					memcpy(&data[offset], buffer, n);
				}
				return n;
			},
			[&](Operation::COperation* obj, double ltotal, double lnow) {
				//Operation::FileImpl* f = (Operation::FileImpl*)obj->GetOperation();
				//std::string path = f->Path();
				std::string path = obj->GetOperation()->Path();
				std::string::size_type pos = path.find_last_of('\\');
				std::string filename = path.substr(pos + 1, -1);
				__TLOG_INFO("下载进度 %.2f%% 路径 %s", (lnow / ltotal) * 100, path.c_str());
				if (lnow == ltotal) {
					obj->GetOperation()->Flush();
					obj->GetOperation()->Close();
					__PLOG_DEBUG("下载完成! 共 %.0f 字节，准备校验...", ltotal);
					char md5[32 + 1] = { 0 };
					MD5Encode32(&data.front(), data.size(), md5, 0);
					if (atol(version["size"].c_str()) == data.size() &&
						strncasecmp(md5, version["md5"].c_str(), strlen(md5)) == 0) {
						__PLOG_DEBUG("校验成功，开始安装新版程序包...");
						std::string content = version["context"];
						utils::_replaceEscChar(content);
						__PLOG_WARN("*******************************************");
						//PLOG_WARN(content.c_str(), version["no"].c_str());
						__PLOG_WARN(content.c_str());
						__PLOG_WARN("*******************************************");
						//::WinExec(path.c_str(), SW_SHOWMAXIMIZED);
						::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
						__LOG_CONSOLE_CLOSE(5000);
						cb(0);//成功，退出
					}
					else {
						__PLOG_ERROR("校验失败，请检查安装包[版本号/大小/MD5值]\n");
						__LOG_CONSOLE_CLOSE(5000);
						cb(1);//失败，继续
					}
				}
			}, NULL, false, stdout) < 0) {
			__PLOG_ERROR("更新失败，失败原因可能：\n\t1.下载包打开失败，目录or文件?\n\t2.下载包可能被占用，请关闭后重试.\n\t3.可能权限不够，请选择其它盘重新安装，不要安装在C盘，或以管理员身份重新启动!\n\t4.检查下载链接%s是否有效!\n\t5.请检查本地网络.", url.c_str());
			__LOG_CONSOLE_CLOSE(10000, true);
			cb(-1);//失败，退出
		}
		__MY_CATCH();
	}

	static inline int _getConfigList(
		Curl::ClientImpl& req,
		utils::INI::ReaderImpl& reader,
		std::map<std::string, std::string>& configs, int& total) {
		configs.clear();
		total = 0;
		int i = 0, n = 0;
		do {
			std::string key = "url." + std::to_string(++i);
			bool hasKey = false;
			std::string config = reader.get("config", key.c_str(), hasKey);
			if (hasKey) {
				if (!config.empty()) {
					++total;
					double size;
					if (req.check(config.c_str(), size) == 0) {
						++n;
						configs[key] = config;
						__TLOG_DEBUG("[有效]%s=%s size=%.0f", key.c_str(), config.c_str(), size);
					}
					else {
						__TLOG_WARN("[失效]%s=%s", key.c_str(), config.c_str());
					}
				}
			}
			else {
				break;
			}
		} while (true);
		return n;
	}

	static inline void _getPlatConfig(utils::INI::ReaderImpl& reader, config_t& conf) {
		conf.server_url = reader.get("config", "SERVER_URL");
		conf.kefu_server_url = reader.get("config", "KEFU_SERVER_URL");
		conf.upload_url = reader.get("config", "UPLOAD_URL");
		conf.app_key = reader.get("config", "APP_KEY");
		conf.secret = reader.get("config", "SECRET");
		conf.app_name = reader.get("config", "APP_NAME");
		conf.app_nickname = reader.get("config", "APP_NICK_NAME");
		conf.nim_data_pc = reader.get("config", "NIM_DATA_PC");
		//__PLOG_INFO("server_url %s", conf.server_url.c_str());
		//__PLOG_INFO("kefu_server_url %s", conf.kefu_server_url.c_str());
		//__PLOG_INFO("upload_url %s", conf.upload_url.c_str());
		//__PLOG_INFO("app_key %s", conf.app_key.c_str());
		//__PLOG_INFO("secret %s", conf.secret.c_str());
		//__PLOG_INFO("app_name %s", conf.app_name.c_str());
		//__PLOG_INFO("app_nickname %s", conf.app_nickname.c_str());
		//__PLOG_INFO("nim_data_pc %s", conf.nim_data_pc.c_str());
	}
	
	static inline void _updateVCRedist(utils::INI::Section& redist, std::string const& dir, std::function<void(int rc)> cb) {
		__MY_TRY();
		std::string url = redist["download"];
		std::string::size_type pos = url.find_last_of('/');
		std::string filename = url.substr(pos + 1, -1);
		if (!utils::_mkDir(dir.c_str())) {
			__PLOG_ERROR("创建下载目录失败..%s\n\t1.可能权限不够，请选择其它盘重新安装，不要安装在C盘，或以管理员身份重新启动!\n\t2.不要安装在中文文件夹", dir.c_str());
			//__LOG_CONSOLE_CLOSE(10000, true);
			//cb(-1);//失败，退出
			return;
		}
		std::string path = dir;
		//std::string path = utils::_GetModulePath() + "/download";
		path += "/" + filename;
		Operation::FileImpl f(path.c_str());
		if (f.Valid()) {
			std::vector<char> data;
			f.Buffer(data);
			f.Close();
			//__PLOG_DEBUG("安装包已存在! 共 %d 字节，准备校验...", data.size());
			if (data.size() > 0) {
				char md5[32 + 1] = { 0 };
				MD5Encode32(&data.front(), data.size(), md5, 0);
				if (atol(redist["size"].c_str()) == data.size() &&
					strncasecmp(md5, redist["md5"].c_str(), strlen(md5)) == 0) {
					__PLOG_DEBUG("校验成功，开始安装...");
					::WinExec(path.c_str(), SW_SHOWNORMAL);
					//::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
					//__LOG_CONSOLE_CLOSE(5000, true);
					//cb(0);//成功，退出
					return;
				}
			}
			//__TLOG_DEBUG("校验失败，重新下载安装包... %s", url.c_str());
		}
		else {
			//__TLOG_DEBUG("开始下载安装包... %s", url.c_str());
		}
		std::vector<char> data;
		Curl::ClientImpl req(true);
		if (req.download(
			url.c_str(),
			path.c_str(),
			[&](Operation::COperation* obj, void* buffer, size_t size, size_t nmemb) -> size_t {
				size_t n = obj->Write(buffer, size, nmemb);
				if (n > 0) {
					int offset = data.size();
					data.resize(offset + n);
					memcpy(&data[offset], buffer, n);
				}
				return n;
			},
			[&](Operation::COperation* obj, double ltotal, double lnow) {
				std::string path = obj->GetOperation()->Path();
				std::string::size_type pos = path.find_last_of('\\');
				std::string filename = path.substr(pos + 1, -1);
				__TLOG_INFO("下载进度 %.2f%% 路径 %s", (lnow / ltotal) * 100, path.c_str());
				if (lnow == ltotal) {
					obj->GetOperation()->Flush();
					obj->GetOperation()->Close();
					//__PLOG_DEBUG("下载完成! 共 %.0f 字节，准备校验...", ltotal);
					char md5[32 + 1] = { 0 };
					MD5Encode32(&data.front(), data.size(), md5, 0);
					if (atol(redist["size"].c_str()) == data.size() &&
						strncasecmp(md5, redist["md5"].c_str(), strlen(md5)) == 0) {
						__PLOG_DEBUG("校验成功，开始安装...");
						::WinExec(path.c_str(), SW_SHOWNORMAL);
						//::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
						//__LOG_CONSOLE_CLOSE(5000, true);
						//cb(0);//成功，退出
					}
					else {
						//__PLOG_ERROR("校验失败，请检查安装包[版本号/大小/MD5值]\n");
						//__LOG_CONSOLE_CLOSE(2000);
						//cb(1);//失败，继续
					}
				}
			}, NULL, false, stdout) < 0) {
			__PLOG_ERROR("更新失败，失败原因可能：\n\t1.下载包打开失败，目录or文件?\n\t2.下载包可能被占用，请关闭后重试.\n\t3.可能权限不够，请选择其它盘重新安装，不要安装在C盘，或以管理员身份重新启动!\n\t4.检查下载链接%s是否有效!\n\t5.请检查本地网络.", url.c_str());
			//__LOG_CONSOLE_CLOSE(10000, true);
			//cb(-1);//失败，退出
		}
		__MY_CATCH();
	}

	//v [IN] 当前版本号
	//name [IN] 7C/WD/NG/1H/28Q/BYQ/WW
	//path [IN] 版本服务器url配置路径
	//dir [IN] 下载安装文件保存路径
	//cb [IN] 回调函数 -1失败，退出 0成功，退出 1失败，继续
	//conf [OUT] 线路配置列表
	void _checkVersion(
		std::string const& v,
		std::string const& name,
		std::string const& path,
		std::string const& dir,
		std::function<void(int rc)> cb,
		config_t& conf) {
		__LOG_CONSOLE_OPEN();
		//版本服务器url
		std::string url;
		{
			//std::string path = utils::_GetModulePath() + "/pc.conf";
			utils::INI::ReaderImpl reader;
			if (reader.parse(path.c_str())) {
				url = reader.get(name.c_str(), "VERSION_URL");
			}
		}
		if (url.empty()) {
			__PLOG_ERROR("VERSION_URL 为空，失败原因可能：\n\t1.请检查 %s\n\t2.可能权限不够，请选择其它盘重新安装，不要安装在C盘，或以管理员身份重新启动!\n\t3.不要安装在中文文件夹", path.c_str());
			__LOG_CONSOLE_CLOSE(10000, true);
			cb(-1);//失败，退出
			return;
		}
		__TLOG_DEBUG("正在检查版本信息 %s", url.c_str());
		__PLOG_DEBUG("当前版本号 %s", v.c_str());
		__MY_TRY();
		//text/plain
		//text/html
		//application/json;charset=utf-8
		//application/octet-stream
		std::list<std::string> header;
		header.push_back("Content-Type: text/plain;charset=utf-8");
		std::string vi;
		Curl::ClientImpl req;
		if (req.get(url.c_str(), &header, &vi) < 0) {
			__PLOG_ERROR("下载失败，失败原因可能：\n\t1.检查链接%s是否有效!\n\t2.请检查本地网络.", url.c_str());
			__LOG_CONSOLE_CLOSE(10000);
			cb(1);//失败，继续
		}
		else {
			//__LOG_WARN(vi.c_str());
			utils::INI::ReaderImpl reader;
			if (reader.parse(vi.c_str(), vi.length())) {
				_getPlatConfig(reader, conf);
#if 1
				int total = 0;
				int n = _getConfigList(req, reader, conf.configs, total);
				if (conf.configs.empty()) {
					__PLOG_ERROR("共检查 %d 条线路，均不可用", total);
					__LOG_CONSOLE_CLOSE(10000, true);
					cb(-1);//失败，退出
					return;
				}
				else {
					__PLOG_DEBUG("共检查 %d 条线路 %d 条可用", total, n);
				}
#endif
				if (utils::_checkVCRedist()) {
					__PLOG_DEBUG("系统已安装 VC 运行时库...");
				}
				else {
					__PLOG_DEBUG("检测到系统未安装 VC 运行时库，准备安装!");
					utils::INI::Section* redist = reader.get("redist");
					_updateVCRedist(*redist, dir, cb);
				}
				utils::INI::Section* version = reader.get("version");
				if (version && v != (*version)["no"]) {
					__PLOG_WARN("发现新版本 %s\n文件大小 %s 字节\nMD5值 %s\n准备更新...",
						(*version)["no"].c_str(),
						(*version)["size"].c_str(),
						(*version)["md5"].c_str());
					utils::_updateVersion(*version, dir, cb);
					return;
				}
				__PLOG_INFO("版本检查完毕，没有发现新版本.");
				__LOG_CONSOLE_CLOSE(5000);
				cb(1);//失败，继续
			}
			else {
				__PLOG_ERROR("版本配置解析失败.");
				__LOG_CONSOLE_CLOSE(10000);
				cb(1);//失败，继续
			}
		}
		__MY_CATCH();
	}
}