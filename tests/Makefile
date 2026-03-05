CXX      = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror
PYTHON   = python3

FRAMEWORK_DIR = framework
UNIT_DIR      = unit
INTEG_DIR     = integration
CONFORM_DIR   = conformance
STRESS_DIR    = stress
STATIC_DIR    = static_analysis
BONUS_DIR     = bonus

# Unit test sources — listed explicitly (no globs)
UNIT_SRCS = \
	$(UNIT_DIR)/test_config_lexer.cpp \
	$(UNIT_DIR)/test_config_parser.cpp \
	$(UNIT_DIR)/test_http_request_parser.cpp \
	$(UNIT_DIR)/test_http_response_builder.cpp \
	$(UNIT_DIR)/test_method_handlers.cpp \
	$(UNIT_DIR)/test_cgi_env.cpp \
	$(UNIT_DIR)/test_chunked_decoder.cpp

UNIT_BINS = $(UNIT_SRCS:.cpp=)

# Build rules
all: static unit integration conformance stress bonus

# --- Static analysis (no binary required) ---
static:
	@echo "=== Static Analysis ==="
	@sh $(STATIC_DIR)/check_constraints.sh

# --- Unit tests ---
unit: $(UNIT_BINS)
	@echo "=== Unit Tests ==="
	@failed=0; \
	for bin in $(UNIT_BINS); do \
		echo "--- Running $$bin ---"; \
		./$$bin || failed=1; \
	done; \
	if [ $$failed -ne 0 ]; then echo "UNIT TESTS FAILED"; exit 1; fi
	@echo "=== Unit Tests Complete ==="

$(UNIT_DIR)/test_config_lexer: $(UNIT_DIR)/test_config_lexer.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $< ../srcs/config/ConfigLexer.cpp

$(UNIT_DIR)/test_config_parser: $(UNIT_DIR)/test_config_parser.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_http_request_parser: $(UNIT_DIR)/test_http_request_parser.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_http_response_builder: $(UNIT_DIR)/test_http_response_builder.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_method_handlers: $(UNIT_DIR)/test_method_handlers.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_cgi_env: $(UNIT_DIR)/test_cgi_env.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

$(UNIT_DIR)/test_chunked_decoder: $(UNIT_DIR)/test_chunked_decoder.cpp $(FRAMEWORK_DIR)/minitest.hpp
	$(CXX) $(CXXFLAGS) -I$(FRAMEWORK_DIR) -I../includes -o $@ $<

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
	rm -f $(UNIT_BINS)

.PHONY: all static unit integration conformance stress bonus clean
