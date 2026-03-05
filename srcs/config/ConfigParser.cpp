#include "ConfigParser.hpp"
#include <cstdlib>
#include <sstream>

std::string ConfigParseException::intToString(int n) {
	std::ostringstream oss;
	oss << n;
	return (oss.str());
}

std::vector<ServerConfig> ConfigParser::parse(const std::string& content) {
	if (content.empty()) {
		throw ConfigParseException("Empty configuration");
	}
	
	ConfigParser parser;
	parser._tokens = ConfigLexer::tokenize(content);
	parser._pos = 0;
	parser.parse();
	parser.validateServers();
	return (parser._servers);
}

ConfigParser::ConfigParser() : _pos(0) {}

Token& ConfigParser::peek() {
	if (_pos < _tokens.size()) {
		return (_tokens[_pos]);
	}
	static Token endToken(END, "", -1);
	return (endToken);
}

Token& ConfigParser::consume() {
	if (_pos < _tokens.size()) {
		return _tokens[_pos++];
	}
	static Token endToken(END, "", -1);
	return (endToken);
}

void ConfigParser::expect(TokenType type, const std::string& expectedValue) {
	Token& tok = peek();
	if (tok.type != type) {
		std::string msg = "Expected " + std::string(
			type == WORD ? "WORD" :
			type == LBRACE ? "LBRACE" :
			type == RBRACE ? "RBRACE" :
			type == SEMICOLON ? "SEMICOLON" : "END"
		);
		throw ConfigParseException(msg, tok.line);
	}
	if (!expectedValue.empty() && tok.value != expectedValue) {
		throw ConfigParseException("Expected '" + expectedValue + "'", tok.line);
	}
	consume();
}

void ConfigParser::expect(TokenType type, const std::string& expectedValue, const std::string& context) {
	Token& tok = peek();
	if (tok.type != type) {
		std::string msg = "Expected " + context;
		throw ConfigParseException(msg, tok.line);
	}
	if (!expectedValue.empty() && tok.value != expectedValue) {
		throw ConfigParseException("Expected '" + expectedValue + "'", tok.line);
	}
	consume();
}

void ConfigParser::parse() {
	while (peek().type != END) {
		expect(WORD, "server", "server block");
		expect(LBRACE);
		ServerConfig server = parseServer();
		_servers.push_back(server);
	}
	
	if (_servers.empty()) {
		throw ConfigParseException("No server blocks found");
	}
}

ServerConfig ConfigParser::parseServer() {
	ServerConfig server;
	bool hasListenDirective = false;
	
	while (peek().type != RBRACE && peek().type != END) {
		if (peek().type == WORD && peek().value == "location") {
			server.locations.push_back(parseLocation());
		} else if (peek().type == WORD) {
			Token& directive = peek();
			std::string dirName = directive.value;
			consume();
			
			if (dirName == "listen") {
				Token& hostPort = peek();
				if (hostPort.type != WORD) {
					throw ConfigParseException("Expected host:port", hostPort.line);
				}
				
				std::string value = hostPort.value;
				size_t colonPos = value.find(':');
				if (colonPos == std::string::npos) {
					throw ConfigParseException("Invalid listen format (expected host:port)", hostPort.line);
				}
				
				server.host = value.substr(0, colonPos);
				server.port = std::atoi(value.substr(colonPos + 1).c_str());
				hasListenDirective = true;
				consume();
				expect(SEMICOLON);
			} else if (dirName == "root" || dirName == "index") {
				if (peek().type != WORD) {
					throw ConfigParseException("Expected value", peek().line);
				}
				consume();
				expect(SEMICOLON);
			} else if (dirName == "client_max_body_size") {
				Token& sizeTok = peek();
				if (sizeTok.type != WORD) {
					throw ConfigParseException("Expected size value", sizeTok.line);
				}
				server.clientMaxBodySize = parseSize(sizeTok.value);
				consume();
				expect(SEMICOLON);
			} else if (dirName == "error_page") {
				Token& codeTok = peek();
				if (codeTok.type != WORD) {
					throw ConfigParseException("Expected status code", codeTok.line);
				}
				int code = std::atoi(codeTok.value.c_str());
				consume();
				
				Token& pathTok = peek();
				if (pathTok.type != WORD) {
					throw ConfigParseException("Expected path", pathTok.line);
				}
				server.errorPages[code] = pathTok.value;
				consume();
				expect(SEMICOLON);
			} else {
				throw ConfigParseException("Unknown directive: " + dirName, directive.line);
			}
		} else {
			throw ConfigParseException("Unexpected token", peek().line);
		}
	}
	
	expect(RBRACE);
	
	if (!hasListenDirective) {
		throw ConfigParseException("Server block missing mandatory 'listen' directive");
	}
	
	return (server);
}

LocationConfig ConfigParser::parseLocation() {
	LocationConfig loc;

	consume();

	Token& pathTok = peek();
	if (pathTok.type != WORD) {
		throw ConfigParseException("Expected location path", pathTok.line);
	}
	loc.path = pathTok.value;
	consume();
	
	expect(LBRACE);
	
	while (peek().type != RBRACE && peek().type != END) {
		if (peek().type != WORD) {
			throw ConfigParseException("Expected directive", peek().line);
		}
		
		Token& directive = peek();
		std::string dirName = directive.value;
		consume();
		
		if (dirName == "root") {
			Token& val = peek();
			if (val.type != WORD) {
				throw ConfigParseException("Expected root path", val.line);
			}
			loc.root = val.value;
			consume();
			expect(SEMICOLON);
		} else if (dirName == "index") {
			Token& val = peek();
			if (val.type != WORD) {
				throw ConfigParseException("Expected index value", val.line);
			}
			loc.index = val.value;
			consume();
			expect(SEMICOLON);
		} else if (dirName == "autoindex") {
			Token& val = peek();
			if (val.type != WORD) {
				throw ConfigParseException("Expected autoindex value", val.line);
			}
			loc.autoindex = (val.value == "on");
			consume();
			expect(SEMICOLON);
		} else if (dirName == "limit_except") {
			while (peek().type != SEMICOLON && peek().type != END) {
				if (peek().type != WORD) {
					throw ConfigParseException("Expected HTTP method", peek().line);
				}
				loc.limitExcept.push_back(peek().value);
				consume();
			}
			expect(SEMICOLON);
		} else if (dirName == "upload_store") {
			Token& val = peek();
			if (val.type != WORD) {
				throw ConfigParseException("Expected upload_store path", val.line);
			}
			loc.uploadStore = val.value;
			consume();
			expect(SEMICOLON);
		} else if (dirName == "cgi_pass") {
			Token& extTok = peek();
			if (extTok.type != WORD) {
				throw ConfigParseException("Expected extension", extTok.line);
			}
			std::string ext = extTok.value;
			consume();
			
			Token& handlerTok = peek();
			if (handlerTok.type != WORD) {
				throw ConfigParseException("Expected handler path", handlerTok.line);
			}
			loc.cgiPass[ext] = handlerTok.value;
			consume();
			expect(SEMICOLON);
		} else {
			throw ConfigParseException("Unknown location directive: " + dirName, directive.line);
		}
	}
	
	expect(RBRACE);
	return (loc);
}

std::size_t ConfigParser::parseSize(const std::string& value) {
	if (value.empty()) {
		return (0);
	}
	
	char suffix = value[value.size() - 1];
	size_t multiplier = 1;
	std::string numPart = value;
	
	if (suffix == 'K' || suffix == 'k') {
		multiplier = 1024;
		numPart = value.substr(0, value.size() - 1);
	} else if (suffix == 'M' || suffix == 'm') {
		multiplier = 1024 * 1024;
		numPart = value.substr(0, value.size() - 1);
	} else if (suffix == 'G' || suffix == 'g') {
		multiplier = 1024 * 1024 * 1024;
		numPart = value.substr(0, value.size() - 1);
	}
	
	return (static_cast<std::size_t>(std::atoi(numPart.c_str())) * multiplier);
}

void ConfigParser::validateServers() {
	std::set<std::string> addresses;
	
	for (size_t i = 0; i < _servers.size(); ++i) {
		std::string addr = _servers[i].host + ":" + ConfigParseException::intToString(_servers[i].port);
		
		if (addresses.find(addr) != addresses.end()) {
			throw ConfigParseException("Duplicate listen address: " + addr);
		}
		addresses.insert(addr);
	}
}
