#ifndef INCLUDE_UTILS_H
#define INCLUDE_UTILS_H

//createUUID 创建uuid
static std::string createUUID() {
	boost::uuids::random_generator rgen;
	boost::uuids::uuid u = rgen();
	std::string uuid;
	uuid.assign(u.begin(), u.end());
	return uuid;
}

//buffer2HexStr
static std::string buffer2HexStr(unsigned char const* buf, size_t len)
{
	std::ostringstream oss;
	oss << std::hex << std::uppercase << std::setfill('0');
	for (size_t i = 0; i < len; ++i) {
		oss << std::setw(2) << (unsigned int)(buf[i]);
	}
	return oss.str();
}

static std::string clearDllPrefix(std::string dllname) {
	size_t nprefix = dllname.find("./");
	if (0 == nprefix) {
		dllname.replace(nprefix, 2, "");
	}
	nprefix = dllname.find("lib");
	if (0 == nprefix)
	{
		dllname.replace(nprefix, 3, "");
	}
	nprefix = dllname.find(".so");
	if (std::string::npos != nprefix) {
		dllname.replace(nprefix, 3, "");
	}
	return (dllname);
}
#endif