#ifndef INCLUDE_SERVLIST_H
#define INCLUDE_SERVLIST_H

#include "Logger/src/Macro.h"

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
	public std::vector<ServItem>,
	public BOOST::Any {
	void bind(BOOST::Json& obj) {
		for (iterator it = begin();
			it != end(); ++it) {
			obj.push_back(*it);
		}
	}
};

void GetGateServList(ServList& servList);

void BroadcastGateUserScore(int64_t userId, int64_t score);

#endif