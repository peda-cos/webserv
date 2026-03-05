#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
	std::string                        path;
	std::string                        root;
	std::string                        index;
	bool                               autoindex;
	std::vector<std::string>           limitExcept;
	std::string                        uploadStore;
	std::map<std::string, std::string> cgiPass;

	LocationConfig() : autoindex(false) {}
};

#endif
