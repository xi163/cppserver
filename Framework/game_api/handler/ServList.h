#ifndef INCLUDE_SERVLIST_H
#define INCLUDE_SERVLIST_H

#include "../rpc/client/RpcContainer.h"

struct ServItem :
	public BOOST::Any {
	ServItem(
		std::string const& Host,
		std::string const& Domain,
		int NumOfLoads)
		: Host(Host)
		, Domain(Domain)
		, NumOfLoads(NumOfLoads) {
	}

	void bind(BOOST::Json& obj, int i) {
		obj.put("host", Host);
		obj.put("domain", Domain);
		obj.put("numOfLoads", NumOfLoads, i);
	}

	int NumOfLoads;
	std::string Host;
	std::string Domain;
};

struct ServList :
	public BOOST::Any,
	public std::vector<ServItem> {

	void bind(BOOST::Json& obj) {
		for (iterator it = begin();
			it != end(); ++it) {
			obj.push_back(*it);
		}
	}
};

void GetServList(ServList& servList, rpc::containTy type, int flags = 0);

void BroadcastGateUserScore(int64_t userId, int64_t score);

#endif