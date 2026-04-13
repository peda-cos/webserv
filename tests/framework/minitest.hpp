#ifndef MINITEST_HPP
#define MINITEST_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

namespace minitest {

struct TestCase {
    const char* suite;
    const char* name;
    void (*func)();
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add(const char* suite, const char* name, void (*func)()) {
        TestCase tc;
        tc.suite = suite;
        tc.name = name;
        tc.func = func;
        _tests.push_back(tc);
    }

    int run() {
        int passed = 0;
        int failed = 0;
        for (size_t i = 0; i < _tests.size(); ++i) {
            _currentFailed = false;
            try {
                _tests[i].func();
            } catch (const AssertionFailure& e) {
                _currentFailed = true;
                std::cout << e.what() << std::endl;
            } catch (const std::exception& e) {
                _currentFailed = true;
                std::cout << "  Unexpected exception: " << e.what() << std::endl;
            } catch (...) {
                _currentFailed = true;
                std::cout << "  Unknown exception caught" << std::endl;
            }
            if (_currentFailed) {
                std::cout << COLOR_RED "✗ " COLOR_RESET 
                          << _tests[i].suite << "." << _tests[i].name << std::endl;
                ++failed;
            } else {
                std::cout << COLOR_GREEN "✓ " COLOR_RESET 
                          << _tests[i].suite << "." << _tests[i].name << std::endl;
                ++passed;
            }
        }
        std::cout << passed << " passed, " << failed << " failed, "
                  << (passed + failed) << " total" << std::endl;
        return (failed > 0) ? 1 : 0;
    }

    struct AssertionFailure {
        std::string msg;
        AssertionFailure(const std::string& m) : msg(m) {}
        const char* what() const { return msg.c_str(); }
    };

private:
    TestRegistry() : _currentFailed(false) {}
    TestRegistry(const TestRegistry&);
    TestRegistry& operator=(const TestRegistry&);

    std::vector<TestCase> _tests;
    bool _currentFailed;
};

struct TestRegistrar {
    TestRegistrar(const char* suite, const char* name, void (*func)()) {
        TestRegistry::instance().add(suite, name, func);
    }
};

} /* namespace minitest */

#define TEST(suite, name) \
    static void minitest_##suite##_##name(); \
    static minitest::TestRegistrar minitest_reg_##suite##_##name( \
        #suite, #name, minitest_##suite##_##name); \
    static void minitest_##suite##_##name()

#define MINITEST_STRINGIFY(x) #x
#define MINITEST_TOSTRING(x) MINITEST_STRINGIFY(x)
#define MINITEST_LOC __FILE__ ":" MINITEST_TOSTRING(__LINE__)

#define ASSERT_EQ(expected, actual) \
    do { \
        if (!((expected) == (actual))) { \
            std::ostringstream oss; \
            oss << "  ASSERT_EQ failed at " << MINITEST_LOC << "\n" \
                << "    Expected: " << (expected) << "\n" \
                << "    Actual:   " << (actual); \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_NE(a, b) \
    do { \
        if ((a) == (b)) { \
            std::ostringstream oss; \
            oss << "  ASSERT_NE failed at " << MINITEST_LOC << "\n" \
                << "    Both values: " << (a); \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::ostringstream oss; \
            oss << "  ASSERT_TRUE failed at " << MINITEST_LOC << "\n" \
                << "    Expression: " << #expr; \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_FALSE(expr) \
    do { \
        if ((expr)) { \
            std::ostringstream oss; \
            oss << "  ASSERT_FALSE failed at " << MINITEST_LOC << "\n" \
                << "    Expression: " << #expr << " was true"; \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_THROWS(expr, ExceptionType) \
    do { \
        bool caught = false; \
        try { \
            expr; \
        } catch (const ExceptionType&) { \
            caught = true; \
        } catch (...) { \
            std::ostringstream oss; \
            oss << "  ASSERT_THROWS failed at " << MINITEST_LOC << "\n" \
                << "    Expected: " << #ExceptionType << "\n" \
                << "    Got: different exception type"; \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
        if (!caught) { \
            std::ostringstream oss; \
            oss << "  ASSERT_THROWS failed at " << MINITEST_LOC << "\n" \
                << "    Expected: " << #ExceptionType << "\n" \
                << "    Got: no exception thrown"; \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

#define ASSERT_STR_CONTAINS(haystack, needle) \
    do { \
        std::string h(haystack); \
        std::string n(needle); \
        if (h.find(n) == std::string::npos) { \
            std::ostringstream oss; \
            oss << "  ASSERT_STR_CONTAINS failed at " << MINITEST_LOC << "\n" \
                << "    Haystack: \"" << h << "\"\n" \
                << "    Needle:   \"" << n << "\""; \
            throw minitest::TestRegistry::AssertionFailure(oss.str()); \
        } \
    } while (0)

/* main() runner — include this in exactly one .cpp per test binary */
#define MINITEST_MAIN() \
    int main() { \
        return minitest::TestRegistry::instance().run(); \
    }

#endif /* MINITEST_HPP */
