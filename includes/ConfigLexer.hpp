#ifndef CONFIGLEXER_HPP
#define CONFIGLEXER_HPP

#include <string>
#include <vector>

enum TokenType {
	WORD,
	LBRACE,
	RBRACE,
	SEMICOLON,
	END
};

struct Token {
	TokenType type;
	std::string value;
	int line;

	Token(TokenType t = END, const std::string& v = "", int l = 1)
		: type(t), value(v), line(l) {}
};

class ConfigLexer {
public:
	static std::vector<Token> tokenize(const std::string& input);
};

#endif
