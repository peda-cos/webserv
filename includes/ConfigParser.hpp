#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ServerConfig.hpp"
#include "ConfigLexer.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <set>

class ConfigParseException : public std::runtime_error {
public:
	explicit ConfigParseException(const std::string& msg)
		: std::runtime_error(msg) {}
	ConfigParseException(const std::string& msg, int line)
		: std::runtime_error(msg + " at line " + intToString(line)) {}
	static std::string intToString(int n);
};

class ConfigParser {
public:
	static std::vector<ServerConfig> parse(const std::string& content);

private:
	ConfigParser();
	
	std::vector<Token> _tokens;
	size_t _pos;
	std::vector<ServerConfig> _servers;
	
	Token& peek();
	Token& consume();
	void expect(TokenType type, const std::string& expectedValue = "");
	void expect(TokenType type, const std::string& expectedValue, const std::string& context);
	
	void parse();
	ServerConfig parseServer();
	LocationConfig parseLocation();
	void parseServerDirective(ServerConfig& server);
	void parseLocationDirective(LocationConfig& loc);
	
	std::size_t parseSize(const std::string& value);
	std::string formatError(const std::string& msg);
	std::string formatError(const std::string& msg, int line);
	bool isDirective(const std::string& value);
	bool isLocationDirective(const std::string& value);
	
	bool hasListen(const ServerConfig& server) const;
	void validateServers();
};

#endif
