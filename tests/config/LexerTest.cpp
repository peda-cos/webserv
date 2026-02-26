#include <iostream>
#include "../../includes/config/Lexer.hpp"

#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << "FAIL: " << #expected << " == " << #actual \
                      << " at line " << __LINE__ \
                      << ": expected " << (expected) \
                      << ", got " << (actual) << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "FAIL: " << #expr << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(name) \
    do { \
        std::cout << "Running " << #name << "... "; \
        if (name()) { \
            std::cout << "PASS" << std::endl; \
            passed++; \
        } else { \
            std::cout << "FAIL" << std::endl; \
            failed++; \
        } \
    } while(0)

bool testBasicSymbols() {
    Lexer lex("{ } ;");
    Token t1 = lex.nextToken();
    ASSERT_EQ(TK_CURLY_OPEN, t1.type);
    ASSERT_TRUE(t1.content == "{");
    
    Token t2 = lex.nextToken();
    ASSERT_EQ(TK_CURLY_CLOSE, t2.type);
    ASSERT_TRUE(t2.content == "}");
    
    Token t3 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t3.type);
    ASSERT_TRUE(t3.content == ";");
    
    Token t4 = lex.nextToken();
    ASSERT_EQ(TK_EOF, t4.type);
    
    ASSERT_TRUE(!lex.hasError());
    return true;
}

bool testDirectives() {
    Lexer lex("listen 8080; server_name example.com;");
    
    Token t1 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t1.type);
    ASSERT_TRUE(t1.content == "listen");
    
    Token t2 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t2.type);
    ASSERT_TRUE(t2.content == "8080");
    
    Token t3 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t3.type);
    
    Token t4 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t4.type);
    ASSERT_TRUE(t4.content == "server_name");
    
    Token t5 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t5.type);
    ASSERT_TRUE(t5.content == "example.com");
    
    Token t6 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t6.type);
    
    Token t7 = lex.nextToken();
    ASSERT_EQ(TK_EOF, t7.type);
    
    ASSERT_TRUE(!lex.hasError());
    return true;
}

bool testComments() {
    Lexer lex("listen 8080; # This is a comment\nserver_name example.com; # Another comment");
    
    Token t1 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t1.type);
    ASSERT_TRUE(t1.content == "listen");
    
    Token t2 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t2.type);
    ASSERT_TRUE(t2.content == "8080");
    
    Token t3 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t3.type);
    
    // Comment should be skipped
    Token t4 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t4.type);
    ASSERT_TRUE(t4.content == "server_name");
    
    Token t5 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t5.type);
    ASSERT_TRUE(t5.content == "example.com");
    
    Token t6 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t6.type);
    
    Token t7 = lex.nextToken();
    ASSERT_EQ(TK_EOF, t7.type);
    
    ASSERT_TRUE(!lex.hasError());
    return true;
}

bool testLineTracking() {
    Lexer lex("listen 8080;\n"
              "server_name example.com;\n"
              "root /var/www;");
    
    Token t1 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t1.type);
    ASSERT_EQ(1u, t1.line);
    
    Token t2 = lex.nextToken();
    ASSERT_EQ(1u, t2.line);
    
    Token t3 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t3.type);
    ASSERT_EQ(1u, t3.line);
    
    Token t4 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t4.type);
    ASSERT_TRUE(t4.content == "server_name");
    ASSERT_EQ(2u, t4.line);
    
    Token t5 = lex.nextToken();
    ASSERT_EQ(2u, t5.line);
    
    Token t6 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t6.type);
    ASSERT_EQ(2u, t6.line);
    
    Token t7 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t7.type);
    ASSERT_TRUE(t7.content == "root");
    ASSERT_EQ(3u, t7.line);
    
    Token t8 = lex.nextToken();
    ASSERT_EQ(TK_WORD, t8.type);
    ASSERT_TRUE(t8.content == "/var/www");
    ASSERT_EQ(3u, t8.line);
    
    Token t9 = lex.nextToken();
    ASSERT_EQ(TK_SEMICOLON, t9.type);
    ASSERT_EQ(3u, t9.line);
    
    ASSERT_TRUE(!lex.hasError());
    return true;
}

bool testQuotedStrings() {
    // Test double quotes
    Lexer lex1("name \"hello world\";");
    Token t1 = lex1.nextToken();
    ASSERT_EQ(TK_WORD, t1.type);
    
    Token t2 = lex1.nextToken();
    ASSERT_EQ(TK_WORD, t2.type);
    ASSERT_TRUE(t2.content == "hello world");
    ASSERT_TRUE(!lex1.hasError());
    
    // Test single quotes
    Lexer lex2("name 'test';");
    Token t3 = lex2.nextToken();
    Token t4 = lex2.nextToken();
    ASSERT_EQ(TK_WORD, t4.type);
    ASSERT_TRUE(t4.content == "test");
    ASSERT_TRUE(!lex2.hasError());
    
    // Test escape sequences
    Lexer lex3("path \"C:\\\\Users\\\\test\";");
    Token t5 = lex3.nextToken();
    Token t6 = lex3.nextToken();
    ASSERT_EQ(TK_WORD, t6.type);
    ASSERT_TRUE(t6.content == "C:\\Users\\test");
    ASSERT_TRUE(!lex3.hasError());
    
    // Test escaped quotes
    Lexer lex4("text \"Say \\\"hello\\\"\";");
    Token t7 = lex4.nextToken();
    Token t8 = lex4.nextToken();
    ASSERT_EQ(TK_WORD, t8.type);
    ASSERT_TRUE(t8.content == "Say \"hello\"");
    ASSERT_TRUE(!lex4.hasError());
    
    // Test newline escape
    Lexer lex5("text \"line1\\nline2\";");
    Token t9 = lex5.nextToken();
    Token t10 = lex5.nextToken();
    ASSERT_EQ(TK_WORD, t10.type);
    ASSERT_TRUE(t10.content == "line1\nline2");
    ASSERT_TRUE(!lex5.hasError());
    
    // Test tab escape
    Lexer lex6("text \"col1\\tcol2\";");
    Token t11 = lex6.nextToken();
    Token t12 = lex6.nextToken();
    ASSERT_EQ(TK_WORD, t12.type);
    ASSERT_TRUE(t12.content == "col1\tcol2");
    ASSERT_TRUE(!lex6.hasError());
    
    return true;
}

bool testErrors() {
    // Test invalid character
    Lexer lex1("listen @8080;");
    Token t1 = lex1.nextToken();
    ASSERT_EQ(TK_WORD, t1.type);
    
    Token t2 = lex1.nextToken();
    ASSERT_EQ(TK_ERROR, t2.type);
    ASSERT_TRUE(lex1.hasError());
    ASSERT_TRUE(lex1.getError().find("Unexpected character") != std::string::npos);
    
    // Test unterminated double quote
    Lexer lex2("name \"unterminated;");
    Token t3 = lex2.nextToken();
    Token t4 = lex2.nextToken();
    ASSERT_EQ(TK_ERROR, t4.type);
    ASSERT_TRUE(lex2.hasError());
    ASSERT_TRUE(lex2.getError() == "Unterminated quoted string");
    
    // Test unterminated single quote
    Lexer lex3("name 'unterminated;");
    Token t5 = lex3.nextToken();
    Token t6 = lex3.nextToken();
    ASSERT_EQ(TK_ERROR, t6.type);
    ASSERT_TRUE(lex3.hasError());
    ASSERT_TRUE(lex3.getError() == "Unterminated quoted string");
    
    // Test newline in quoted string
    Lexer lex4("name \"line1\nline2\";");
    Token t7 = lex4.nextToken();
    Token t8 = lex4.nextToken();
    ASSERT_EQ(TK_ERROR, t8.type);
    ASSERT_TRUE(lex4.hasError());
    ASSERT_TRUE(lex4.getError() == "Unterminated quoted string");
    
    // Test invalid escape sequence
    Lexer lex5("path \"\\x\";");
    Token t9 = lex5.nextToken();
    Token t10 = lex5.nextToken();
    ASSERT_EQ(TK_ERROR, t10.type);
    ASSERT_TRUE(lex5.hasError());
    ASSERT_TRUE(lex5.getError().find("Invalid escape sequence") != std::string::npos);
    
    return true;
}

int main() {
    int passed = 0, failed = 0;
    
    RUN_TEST(testBasicSymbols);
    RUN_TEST(testDirectives);
    RUN_TEST(testComments);
    RUN_TEST(testLineTracking);
    RUN_TEST(testQuotedStrings);
    RUN_TEST(testErrors);
    
    std::cout << "\nResults: " << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
