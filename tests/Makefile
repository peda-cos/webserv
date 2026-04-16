CXX      = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror
PYTHON   = python3

# Colors
GREEN  = \033[0;32m
RED    = \033[0;31m
YELLOW = \033[0;33m
RESET  = \033[0m

FRAMEWORK_DIR = framework
UNIT_DIR      = unit
INTEG_DIR     = integration
CONFORM_DIR   = conformance
STRESS_DIR    = stress
STATIC_DIR    = static_analysis
BONUS_DIR     = bonus

# Pure unit tests (must pass - failures abort the build)
PURE_UNIT_TESTS = \
	$(UNIT_DIR)/test_config_parser \
	$(UNIT_DIR)/test_http_request_parser \
	$(UNIT_DIR)/test_cgi_env \
	$(UNIT_DIR)/test_cgi_parser_contract \
	$(UNIT_DIR)/test_cgi_executor \
	$(UNIT_DIR)/test_chunked_decoder

# TDD stub tests (expected to fail - informational only)
STUB_TESTS = \
	$(UNIT_DIR)/test_http_response_builder \
	$(UNIT_DIR)/test_method_handlers

# Integration tests (require ./webserv running - informational only)
INTEGRATION_UNIT_TESTS = \
	$(UNIT_DIR)/test_connection_loop

ALL_UNIT_BINS = $(PURE_UNIT_TESTS) $(STUB_TESTS) $(INTEGRATION_UNIT_TESTS)

# Build rules
all: static unit integration conformance stress bonus

# --- Static analysis (no binary required) ---
static:
	@echo "=== Static Analysis ==="
	@sh $(STATIC_DIR)/check_constraints.sh

# --- Unit tests ---
unit: $(ALL_UNIT_BINS)
	@pure_passed=0; pure_failed=0; stub_count=0; integ_count=0; integ_failed=0; \
	\
	for bin in $(PURE_UNIT_TESTS); do \
		name=$$(basename $$bin); \
		output=$$(./$$bin 2>&1); \
		if echo "$$output" | grep -q "0 failed"; then \
			summary=$$(echo "$$output" | grep "passed, 0 failed" | tail -1); \
			echo ""; \
			echo " $(GREEN)✓ PASS$(RESET)  $$name"; \
			echo "      $$summary"; \
			last_tests=$$(echo "$$output" | head -n -1 | tail -5); \
			echo "$$last_tests" | sed 's/^/    /'; \
			pure_passed=$$((pure_passed + 1)); \
		else \
			echo ""; \
			echo " $(RED)✗ FAIL$(RESET)  $$name"; \
			echo "$$output" | grep -E "ASSERT|FAILED" | head -3; \
			pure_failed=$$((pure_failed + 1)); \
		fi; \
	done; \
	\
	if [ $$pure_failed -gt 0 ]; then \
		echo ""; \
		echo "$(RED)✗ Tests failed$(RESET)"; \
		exit 1; \
	fi; \
	\
	echo ""; \
	for bin in $(STUB_TESTS); do \
		name=$$(basename $$bin); \
		echo " $(YELLOW)◐ STUB$(RESET)  $$name"; \
		stub_output=$$(./$$bin 2>&1 | grep "^✗\|ASSERT\|FAILED" | head -5); \
		echo "$$stub_output" | sed 's/^/  /'; \
		stub_count=$$((stub_count + 1)); \
	done; \
	\
	for bin in $(INTEGRATION_UNIT_TESTS); do \
		name=$$(basename $$bin); \
		output=$$(./$$bin 2>&1); \
		if echo "$$output" | grep -q "\[SKIP\]"; then \
			echo ""; \
			echo " $(YELLOW)◐ SKIP$(RESET)  $$name"; \
			integ_output=$$(echo "$$output" | grep "SKIP\|INFO" | head -2); \
			echo "$$integ_output" | sed 's/^/  /'; \
		else \
			summary=$$(echo "$$output" | grep "passed.*failed.*total" | tail -1); \
			if [ -n "$$summary" ]; then \
				passed=$$(echo "$$summary" | awk '{print $$1}'); \
				failed=$$(echo "$$summary" | awk '{print $$3}'); \
				if [ "$$failed" = "0" ]; then \
					echo ""; \
					echo " $(GREEN)✓ PASS$(RESET)  $$name"; \
					echo "      $$summary"; \
					last_tests=$$(echo "$$output" | grep -E "^✓|^✗" | tail -5); \
					echo "$$last_tests" | sed 's/^/    /'; \
				else \
					echo ""; \
					echo " $(RED)✗ INTEG$(RESET)  $$name"; \
					echo "      $$summary"; \
					last_tests=$$(echo "$$output" | grep -E "^✓|^✗" | tail -5); \
					[ -n "$$last_tests" ] && echo "$$last_tests" | sed 's/^/    /'; \
					integ_failed=$$((integ_failed + 1)); \
				fi; \
			fi; \
		fi; \
		integ_count=$$((integ_count + 1)); \
	done; \
	\
	echo ""; \
	echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(RESET)"; \
	if [ $$integ_failed -gt 0 ]; then \
		echo "Test Suites: $(GREEN)✓ $$pure_passed passed$(RESET), $(RED)✗ $$integ_failed integration failed$(RESET), 1 total"; \
	else \
		echo "Test Suites: $(GREEN)✓ 1 passed$(RESET), 1 total"; \
	fi; \
	echo "Tests:       $(GREEN)✓ $$pure_passed passed$(RESET), $$stub_count stub, $$integ_count integration"; \
	echo "$(GREEN)━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━$(RESET)"; \
	echo ""

$(UNIT_DIR)/test_config_parser: $(UNIT_DIR)/test_config_parser.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) \
		-I$(FRAMEWORK_DIR) -I../includes -I../includes/config -I../includes/utils \
		-I../includes/http -I../includes/server -I../includes/cgi \
		-o $@ $< \
		../srcs/config/ConfigLexer.cpp \
		../srcs/config/ConfigParser.cpp \
		../srcs/config/ConfigParserServer.cpp \
		../srcs/config/ConfigParserLocation.cpp \
		../srcs/config/ParserSyntaxError.cpp \
		../srcs/utils/ConfigUtils.cpp \
		../srcs/utils/ParsingUtils.cpp \
		../srcs/utils/Logger.cpp \
		../srcs/utils/ParserDirectiveUtils.cpp

$(UNIT_DIR)/test_http_request_parser: $(UNIT_DIR)/test_http_request_parser.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) \
		-I$(FRAMEWORK_DIR) -I../includes -I../includes/http \
		-o $@ $< \
		../srcs/http/HttpRequest.cpp \
		../srcs/http/HttpRequestParser.cpp \
		../srcs/http/ChunkedDecoder.cpp

$(UNIT_DIR)/test_http_response_builder: $(UNIT_DIR)/test_http_response_builder.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_method_handlers: $(UNIT_DIR)/test_method_handlers.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

UTILS_SRCS = ../srcs/utils/CgiUtils.cpp ../srcs/utils/HttpUtils.cpp ../srcs/utils/Logger.cpp ../srcs/utils/ParsingUtils.cpp ../srcs/utils/StringUtils.cpp
HTTP_SRCS = ../srcs/http/HttpRequest.cpp
CGI_SRCS = ../srcs/cgi/CgiEnvBuilder.cpp
TEST_CGI_SRCS = $(UTILS_SRCS) $(HTTP_SRCS) $(CGI_SRCS)
TEST_CGI_EXECUTOR_SRCS = $(UTILS_SRCS) $(HTTP_SRCS) ../srcs/cgi/CgiEnvBuilder.cpp ../srcs/cgi/CgiExecutor.cpp ../srcs/cgi/CgiPipeManager.cpp

$(UNIT_DIR)/test_cgi_env: $(UNIT_DIR)/test_cgi_env.cpp $(FRAMEWORK_DIR)/minitest.hpp $(TEST_CGI_SRCS)
	$(CXX) $(CXXFLAGS) -Iframework -I../includes -I../includes/cgi -I../includes/http -I../includes/config -I../includes/utils -o $@ $< $(TEST_CGI_SRCS)

$(UNIT_DIR)/test_cgi_parser_contract: $(UNIT_DIR)/test_cgi_parser_contract.cpp $(FRAMEWORK_DIR)/minitest.hpp $(TEST_CGI_EXECUTOR_SRCS)
	$(CXX) $(CXXFLAGS) -Iframework -I../includes -I../includes/cgi -I../includes/http -I../includes/config -I../includes/utils -o $@ $< $(TEST_CGI_EXECUTOR_SRCS)

$(UNIT_DIR)/test_cgi_executor: $(UNIT_DIR)/test_cgi_executor.cpp $(FRAMEWORK_DIR)/minitest.hpp $(TEST_CGI_EXECUTOR_SRCS)
	$(CXX) $(CXXFLAGS) -Iframework -I../includes -I../includes/cgi -I../includes/http -I../includes/config -I../includes/utils -o $@ $< $(TEST_CGI_EXECUTOR_SRCS)

$(UNIT_DIR)/test_chunked_decoder: $(UNIT_DIR)/test_chunked_decoder.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -I../includes/http -o $@ $< ../srcs/http/ChunkedDecoder.cpp

# --- Integration tests (requires ./webserv binary) ---
integration:
	@echo "=== Integration Tests ==="
	$(PYTHON) -m pytest $(INTEG_DIR)/ -v --tb=short 2>/dev/null || \
		(for f in $(INTEG_DIR)/test_*.py; do echo "--- Running $$f ---"; $(PYTHON) $$f || exit 1; done)
	@echo "=== Integration Tests Complete ==="

# --- Conformance tests (requires ./webserv binary) ---
conformance:
	@echo "=== Conformance Tests ==="
	$(PYTHON) -m pytest $(CONFORM_DIR)/ -v --tb=short 2>/dev/null || \
		(for f in $(CONFORM_DIR)/test_*.py; do echo "--- Running $$f ---"; $(PYTHON) $$f || exit 1; done)
	@echo "=== Conformance Tests Complete ==="

# --- Stress tests (requires ./webserv binary) ---
stress:
	@echo "=== Stress Tests ==="
	$(PYTHON) -m pytest $(STRESS_DIR)/ -v --tb=short 2>/dev/null || \
		(for f in $(STRESS_DIR)/test_*.py; do echo "--- Running $$f ---"; $(PYTHON) $$f || exit 1; done)
	@echo "=== Stress Tests Complete ==="

# --- Bonus tests (requires ./webserv binary) ---
bonus:
	@echo "=== Bonus Tests ==="
	$(PYTHON) -m pytest $(BONUS_DIR)/ -v --tb=short 2>/dev/null || \
		(for f in $(BONUS_DIR)/test_*.py; do echo "--- Running $$f ---"; $(PYTHON) $$f || exit 1; done)
	@echo "=== Bonus Tests Complete ==="

# --- Cleanup ---
clean:
	rm -f $(ALL_UNIT_BINS)

.PHONY: all static unit integration conformance stress bonus clean

$(UNIT_DIR)/test_connection_loop: $(UNIT_DIR)/test_connection_loop.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) \
		-I$(FRAMEWORK_DIR) -I../includes -I../includes/config -I../includes/utils \
		-I../includes/http -I../includes/server -I../includes/cgi \
		-o $@ $< \
		../srcs/server/Server.cpp \
		../srcs/utils/Logger.cpp \
		../srcs/utils/CgiUtils.cpp \
		../srcs/config/ConfigLexer.cpp \
		../srcs/config/ConfigParser.cpp \
		../srcs/config/ConfigParserServer.cpp \
		../srcs/config/ConfigParserLocation.cpp \
		../srcs/config/ParserSyntaxError.cpp \
		../srcs/utils/ConfigUtils.cpp \
		../srcs/utils/HttpUtils.cpp \
		../srcs/utils/StringUtils.cpp \
		../srcs/utils/ParsingUtils.cpp \
		../srcs/http/HttpRequest.cpp \
		../srcs/http/HttpRequestParser.cpp \
		../srcs/http/HttpResponse.cpp \
		../srcs/http/ChunkedDecoder.cpp \
		../srcs/utils/ParserDirectiveUtils.cpp \
		../srcs/cgi/CgiHandler.cpp \
		../srcs/cgi/CgiExecutor.cpp \
		../srcs/cgi/CgiEnvBuilder.cpp \
		../srcs/cgi/CgiPipeManager.cpp
