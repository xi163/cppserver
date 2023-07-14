#pragma once

#include "../Macro.h"

namespace utils {
	
	struct config_t {
		std::string server_url, kefu_server_url;
		std::string upload_url;
		std::string app_key, secret;
		std::string app_name;
		std::string app_nickname;
		std::string nim_data_pc;
		std::map<std::string, std::string> configs;
	};

	//v [IN] 当前版本号
	//name [IN] 7C/WD/NG/1H/28Q/BYQ/WW
	//path [IN] 版本服务器url配置路径
	//dir [IN] 下载安装文件保存路径
	//cb [IN] 回调函数 -1失败，退出 0成功，退出 1失败，继续
	//conf [OUT] 线路配置列表
	void checkVersion(
		std::string const& v,
		std::string const& name,
		std::string const& path,
		std::string const& dir,
		std::function<void(int rc)> cb,
		config_t& conf);
}