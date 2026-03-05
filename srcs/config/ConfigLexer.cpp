#include "ConfigLexer.hpp"
#include <cctype>

static bool isWhitespace(char c) {
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static bool isPunctuation(char c) {
	return (c == '{' || c == '}' || c == ';');
}

std::vector<Token> ConfigLexer::tokenize(const std::string& input) {
	std::vector<Token> tokens;
	std::string buffer;
	int line = 1;
	bool inComment = false;

	for (size_t i = 0; i < input.length(); ++i) {
		char c = input[i];

		if (c == '\n') {
			if (!buffer.empty()) {
				tokens.push_back(Token(WORD, buffer, line));
				buffer.clear();
			}
			inComment = false;
			line++;
			continue;
		}

		if (inComment) {
			continue;
		}

		if (c == '#') {
			if (!buffer.empty()) {
				tokens.push_back(Token(WORD, buffer, line));
				buffer.clear();
			}
			inComment = true;
		} else if (isWhitespace(c)) {
			if (!buffer.empty()) {
				tokens.push_back(Token(WORD, buffer, line));
				buffer.clear();
			}
		} else if (isPunctuation(c)) {
			if (!buffer.empty()) {
				tokens.push_back(Token(WORD, buffer, line));
				buffer.clear();
			}
			if (c == '{') {
				tokens.push_back(Token(LBRACE, std::string(1, c), line));
			} else if (c == '}') {
				tokens.push_back(Token(RBRACE, std::string(1, c), line));
			} else if (c == ';') {
				tokens.push_back(Token(SEMICOLON, std::string(1, c), line));
			}
		} else {
			buffer += c;
		}
	}

	if (!buffer.empty()) {
		tokens.push_back(Token(WORD, buffer, line));
	}

	tokens.push_back(Token(END, "", line));
	return (tokens);
}
