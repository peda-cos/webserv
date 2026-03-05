#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "LocationConfig.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstddef>

struct ServerConfig {
	std::string                     host;
	int                             port;
	std::size_t                     clientMaxBodySize;
	std::map<int, std::string>      errorPages;
	std::vector<LocationConfig>     locations;

	ServerConfig() : port(0), clientMaxBodySize(0) {}
};

#endif
