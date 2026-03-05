#include "minitest.hpp"
#include "ConfigLexer.hpp"

#include <iostream>

TEST(ConfigLexer, SimpleServerBlock) {
	std::string input = "server { listen 127.0.0.1:8080; }";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(7, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("server"), tokens[0].value);
	ASSERT_EQ(LBRACE, tokens[1].type);
	ASSERT_EQ(WORD, tokens[2].type);
	ASSERT_EQ(std::string("listen"), tokens[2].value);
	ASSERT_EQ(WORD, tokens[3].type);
	ASSERT_EQ(std::string("127.0.0.1:8080"), tokens[3].value);
	ASSERT_EQ(SEMICOLON, tokens[4].type);
	ASSERT_EQ(RBRACE, tokens[5].type);
	ASSERT_EQ(END, tokens[6].type);
}

TEST(ConfigLexer, LineTracking) {
	std::string input = "server {\n    listen 8080\n}";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(6, static_cast<int>(tokens.size()));
	ASSERT_EQ(1, tokens[0].line);
	ASSERT_EQ(std::string("server"), tokens[0].value);
	ASSERT_EQ(2, tokens[2].line);
	ASSERT_EQ(std::string("listen"), tokens[2].value);
	ASSERT_EQ(2, tokens[3].line);
	ASSERT_EQ(std::string("8080"), tokens[3].value);
	ASSERT_EQ(3, tokens[4].line);
	ASSERT_EQ(std::string("}"), tokens[4].value);
}

TEST(ConfigLexer, FullLineComment) {
	std::string input = "# this is a comment\nserver {";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(3, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("server"), tokens[0].value);
	ASSERT_EQ(LBRACE, tokens[1].type);
	ASSERT_EQ(END, tokens[2].type);
}

TEST(ConfigLexer, InlineComment) {
	std::string input = "listen 8080; # port number";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(4, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("listen"), tokens[0].value);
	ASSERT_EQ(WORD, tokens[1].type);
	ASSERT_EQ(std::string("8080"), tokens[1].value);
	ASSERT_EQ(SEMICOLON, tokens[2].type);
	ASSERT_EQ(END, tokens[3].type);
}

TEST(ConfigLexer, CommentPreservesLineCount) {
	std::string input = "# comment\nlisten 8080;";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(4, static_cast<int>(tokens.size()));
	ASSERT_EQ(2, tokens[0].line);
	ASSERT_EQ(std::string("listen"), tokens[0].value);
}

TEST(ConfigLexer, AdjacentPunctuation) {
	std::string input = "server{listen 8080;}";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(7, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("server"), tokens[0].value);
	ASSERT_EQ(LBRACE, tokens[1].type);
	ASSERT_EQ(WORD, tokens[2].type);
	ASSERT_EQ(std::string("listen"), tokens[2].value);
	ASSERT_EQ(WORD, tokens[3].type);
	ASSERT_EQ(std::string("8080"), tokens[3].value);
	ASSERT_EQ(SEMICOLON, tokens[4].type);
	ASSERT_EQ(RBRACE, tokens[5].type);
	ASSERT_EQ(END, tokens[6].type);
}

TEST(ConfigLexer, EmptyInput) {
	std::string input = "";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(1, static_cast<int>(tokens.size()));
	ASSERT_EQ(END, tokens[0].type);
	ASSERT_EQ(std::string(""), tokens[0].value);
}

TEST(ConfigLexer, WhitespaceOnlyInput) {
	std::string input = "   \n\t  \n";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(1, static_cast<int>(tokens.size()));
	ASSERT_EQ(END, tokens[0].type);
}

TEST(ConfigLexer, MultipleSpacesBetweenWords) {
	std::string input = "listen    8080;";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(4, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("listen"), tokens[0].value);
	ASSERT_EQ(WORD, tokens[1].type);
	ASSERT_EQ(std::string("8080"), tokens[1].value);
	ASSERT_EQ(SEMICOLON, tokens[2].type);
	ASSERT_EQ(END, tokens[3].type);
}

TEST(ConfigLexer, TabsAndMixedWhitespace) {
	std::string input = "\tlisten\t\t8080\t;";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(4, static_cast<int>(tokens.size()));
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("listen"), tokens[0].value);
	ASSERT_EQ(WORD, tokens[1].type);
	ASSERT_EQ(std::string("8080"), tokens[1].value);
	ASSERT_EQ(SEMICOLON, tokens[2].type);
	ASSERT_EQ(END, tokens[3].type);
}

TEST(ConfigLexer, ComplexConfig) {
	std::string input =
		"server {\n"
		"    listen 127.0.0.1:8080;\n"
		"    root /var/www/html;\n"
		"    index index.html;\n"
		"}\n";
	std::vector<Token> tokens = ConfigLexer::tokenize(input);

	ASSERT_EQ(13, static_cast<int>(tokens.size()));

	// Verify key tokens
	ASSERT_EQ(WORD, tokens[0].type);
	ASSERT_EQ(std::string("server"), tokens[0].value);
	ASSERT_EQ(1, tokens[0].line);

	ASSERT_EQ(LBRACE, tokens[1].type);
	ASSERT_EQ(1, tokens[1].line);

	ASSERT_EQ(WORD, tokens[2].type);
	ASSERT_EQ(std::string("listen"), tokens[2].value);
	ASSERT_EQ(2, tokens[2].line);

	ASSERT_EQ(WORD, tokens[5].type);
	ASSERT_EQ(std::string("root"), tokens[5].value);
	ASSERT_EQ(3, tokens[5].line);

	ASSERT_EQ(WORD, tokens[6].type);
	ASSERT_EQ(std::string("/var/www/html"), tokens[6].value);
	ASSERT_EQ(3, tokens[6].line);

	ASSERT_EQ(RBRACE, tokens[11].type);
	ASSERT_EQ(5, tokens[11].line);

	ASSERT_EQ(END, tokens[12].type);
}

MINITEST_MAIN()
