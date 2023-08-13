#ifndef INCLUDE_ROOMCONTAINER_H
#define INCLUDE_ROOMCONTAINER_H

#include "public/Inc.h"

struct RoomContainer {
	void random_game_server_ipport(uint32_t roomid, std::string& ipport);
	
	void add(std::vector<std::string> const& names);

	void process(std::vector<std::string> const& names);
private:
	void add(std::string const& name);
	void remove(std::string const& name);
public:
	std::vector<std::string> names_;
	mutable boost::shared_mutex mutex_;
	std::map<int, std::set<std::string>> room_servers_;
	mutable boost::shared_mutex room_servers_mutex_;
};

#endif