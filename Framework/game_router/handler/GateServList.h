#ifndef INCLUDE_GATESERVLIST_H
#define INCLUDE_GATESERVLIST_H

#include "Logger/src/Macro.h"

struct GateServItem :
	public BOOST::Any {
	GateServItem(
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

struct GateServList :
	public std::vector<GateServItem>,
	public BOOST::Any {
	void bind(BOOST::Json& obj) {
		for (iterator it = begin();
			it != end(); ++it) {
			obj.push_back(*it);
		}
	}
};

void GetGateServList(GateServList& servList);

void BroadcastGateUserScore(int64_t userId, int64_t score);

#endif