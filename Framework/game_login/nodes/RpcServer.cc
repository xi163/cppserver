#include "proto/Game.Common.pb.h"

#include "../Login.h"

bool LoginServ::onRpcCondition(const muduo::net::InetAddress& peerAddr, muduo::net::InetRegion& peerRegion) {
	std::string ipaddr = peerAddr.toIp();
	//dead loop bug???
	ipLocator_.GetAddressByIp(ipaddr.c_str(), peerRegion.location, peerRegion.country);
	//country
	for (std::vector<std::string>::const_iterator it = country_list_.begin();
		it != country_list_.end(); ++it) {
		if (peerRegion.country.find(*it) != std::string::npos) {
			//location
			for (std::vector<std::string>::const_iterator ir = location_list_.begin();
				ir != location_list_.end(); ++ir) {
				if (peerRegion.location.find(*ir) != std::string::npos) {
					Infof("%s %s %s [√]通过", ipaddr.c_str(), peerRegion.country.c_str(), peerRegion.location.c_str());
					return true;
				}
			}
			break;
		}
	}
	Infof("%s %s %s [×]禁止访问", ipaddr.c_str(), peerRegion.country.c_str(), peerRegion.location.c_str());
	return false;
}