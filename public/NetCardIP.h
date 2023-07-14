#ifndef INCLUDE_NETCARDIP_H
#define INCLUDE_NETCARDIP_H

#define MAXBUFSZ 1024

static int IpByNetCardName(std::string const& netName, std::string& strIP) {
	int sockfd;
	struct ifconf conf;
	struct ifreq* ifr;
	char buff[MAXBUFSZ] = { 0 };
	int num;
	int i;
	sockfd = ::socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	conf.ifc_len = MAXBUFSZ;
	conf.ifc_buf = buff;
	if (::ioctl(sockfd, SIOCGIFCONF, &conf) < 0) {
		::close(sockfd);
		return -1;
	}
	num = conf.ifc_len / sizeof(struct ifreq);
	ifr = conf.ifc_req;

	for (int i = 0; i < num; ++i) {
		struct sockaddr_in* sin = (struct sockaddr_in*)(&ifr->ifr_addr);
		if (::ioctl(sockfd, SIOCGIFFLAGS, ifr) < 0) {
			::close(sockfd);
			return -1;
		}
		if ((ifr->ifr_flags & IFF_UP) &&
			strcmp(netName.c_str(), ifr->ifr_name) == 0) {
			strIP = ::inet_ntoa(sin->sin_addr);
			::close(sockfd);
			return 0;
		}

		++ifr;
	}
	::close(sockfd);
	return -1;
}

#endif