#!/bin/sh
set -e

# ============================================================================
# check_constraints.sh — Static analysis for webserv hard constraints
#
# Enforces rules from AGENTS.md. POSIX sh only — no bashisms.
# Exit 0 if all checks pass, exit 1 if any violation found.
# ============================================================================

violations=0

# --------------------------------------------------------------------------
# Determine project root
# --------------------------------------------------------------------------
if [ -d "srcs" ]; then
    ROOT="."
elif [ -d "../srcs" ]; then
    ROOT=".."
else
    echo "WARNING: srcs/ directory not found — nothing to check yet."
    echo "All checks passed (no sources to analyse)."
    exit 0
fi

# --------------------------------------------------------------------------
# Helper: report a single violation
#   report <file> <line> <rule> <detail>
# --------------------------------------------------------------------------
report() {
    _file="$1"
    _line="$2"
    _rule="$3"
    _detail="$4"
    echo "VIOLATION [$_rule] $_file:$_line — $_detail"
    violations=$((violations + 1))
}

# ==========================================================================
# 1. SINGLE_POLL_INSTANCE
#    Exactly 0 or 1 call sites to poll(, epoll_wait(, or kevent( in srcs/
# ==========================================================================
echo "--- Checking SINGLE_POLL_INSTANCE ---"

poll_hits=""
poll_hits=$(grep -rn -E 'poll\(|epoll_wait\(|kevent\(' "$ROOT/srcs/" --include='*.cpp' 2>/dev/null || true)

poll_count=0
if [ -n "$poll_hits" ]; then
    poll_count=$(printf '%s\n' "$poll_hits" | wc -l | tr -d ' ')
fi

if [ "$poll_count" -gt 1 ]; then
    echo "FAIL: Found $poll_count poll/epoll/kevent call sites (max 1 allowed):"
    printf '%s\n' "$poll_hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        report "$_file" "$_line" "SINGLE_POLL_INSTANCE" "extra poll/epoll/kevent call site"
    done
    # The while-read in a pipe runs in a subshell, so we must count here too
    violations=$((violations + poll_count - 1))
else
    echo "PASS ($poll_count call site(s) found)"
fi

# ==========================================================================
# 2. FORK_ONLY_IN_CGI
#    All fork( calls must reside under srcs/cgi/
# ==========================================================================
echo "--- Checking FORK_ONLY_IN_CGI ---"

fork_outside=""
fork_outside=$(grep -rn 'fork(' "$ROOT/srcs/" --include='*.cpp' 2>/dev/null \
    | grep -v "$ROOT/srcs/cgi/" || true)

if [ -n "$fork_outside" ]; then
    printf '%s\n' "$fork_outside" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [FORK_ONLY_IN_CGI] $_file:$_line — fork() outside srcs/cgi/"
    done
    _cnt=$(printf '%s\n' "$fork_outside" | wc -l | tr -d ' ')
    violations=$((violations + _cnt))
    echo "FAIL: $_cnt fork() call(s) outside srcs/cgi/"
else
    echo "PASS"
fi

# ==========================================================================
# 3. NO_ERRNO_AFTER_READWRITE
#    errno must not appear on the line immediately after read(/write(/recv(/send(
# ==========================================================================
echo "--- Checking NO_ERRNO_AFTER_READWRITE ---"

errno_fail=0
find "$ROOT/srcs" -name '*.cpp' -type f | while IFS= read -r cppfile; do
    # Use awk to detect errno on the line right after a read/write/recv/send call
    awk '
    /read\(/ || /write\(/ || /recv\(/ || /send\(/ {
        prev_is_rw = 1
        prev_line = NR
        next
    }
    prev_is_rw && /errno/ {
        print FILENAME ":" NR ":errno after read/write at line " prev_line
    }
    { prev_is_rw = 0 }
    ' "$cppfile"
done > "$ROOT/.errno_check_tmp" 2>/dev/null || true

if [ -s "$ROOT/.errno_check_tmp" ]; then
    while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        _detail=$(printf '%s' "$hit" | cut -d: -f3-)
        echo "VIOLATION [NO_ERRNO_AFTER_READWRITE] $_file:$_line — $_detail"
        errno_fail=$((errno_fail + 1))
    done < "$ROOT/.errno_check_tmp"
    violations=$((violations + errno_fail))
    echo "FAIL: $errno_fail errno-after-read/write occurrence(s)"
else
    echo "PASS"
fi
rm -f "$ROOT/.errno_check_tmp"

# ==========================================================================
# 4. NO_CPP11_SYNTAX
#    Forbid nullptr, auto type-deduction, C++11 headers, std::thread/mutex,
#    and initializer-list assignment = { ... }
# ==========================================================================
echo "--- Checking NO_CPP11_SYNTAX ---"

cpp11_fail=0

# 4a. nullptr (word boundary)
hits=$(grep -rn '\bnullptr\b' "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.cpp' --include='*.hpp' 2>/dev/null || true)
if [ -n "$hits" ]; then
    printf '%s\n' "$hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_CPP11_SYNTAX] $_file:$_line — use of nullptr"
    done
    _cnt=$(printf '%s\n' "$hits" | wc -l | tr -d ' ')
    cpp11_fail=$((cpp11_fail + _cnt))
fi

# 4b. auto type deduction: "auto " followed by an identifier
hits=$(grep -rn '\bauto [a-zA-Z_]' "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.cpp' --include='*.hpp' 2>/dev/null || true)
if [ -n "$hits" ]; then
    printf '%s\n' "$hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_CPP11_SYNTAX] $_file:$_line — auto type deduction"
    done
    _cnt=$(printf '%s\n' "$hits" | wc -l | tr -d ' ')
    cpp11_fail=$((cpp11_fail + _cnt))
fi

# 4c. C++11 headers
hits=$(grep -rn '#include <thread>\|#include <mutex>\|#include <chrono>' \
    "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.cpp' --include='*.hpp' 2>/dev/null || true)
if [ -n "$hits" ]; then
    printf '%s\n' "$hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_CPP11_SYNTAX] $_file:$_line — C++11 header"
    done
    _cnt=$(printf '%s\n' "$hits" | wc -l | tr -d ' ')
    cpp11_fail=$((cpp11_fail + _cnt))
fi

# 4d. std::thread / std::mutex
hits=$(grep -rn 'std::thread\|std::mutex' "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.cpp' --include='*.hpp' 2>/dev/null || true)
if [ -n "$hits" ]; then
    printf '%s\n' "$hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_CPP11_SYNTAX] $_file:$_line — std::thread or std::mutex"
    done
    _cnt=$(printf '%s\n' "$hits" | wc -l | tr -d ' ')
    cpp11_fail=$((cpp11_fail + _cnt))
fi

# 4e. Initializer list heuristic: = {
hits=$(grep -rn '= {' "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.cpp' --include='*.hpp' 2>/dev/null || true)
if [ -n "$hits" ]; then
    printf '%s\n' "$hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_CPP11_SYNTAX] $_file:$_line — possible initializer list (= {)"
    done
    _cnt=$(printf '%s\n' "$hits" | wc -l | tr -d ' ')
    cpp11_fail=$((cpp11_fail + _cnt))
fi

if [ "$cpp11_fail" -gt 0 ]; then
    violations=$((violations + cpp11_fail))
    echo "FAIL: $cpp11_fail C++11+ syntax occurrence(s)"
else
    echo "PASS"
fi

# ==========================================================================
# 5. NO_PRAGMA_ONCE
#    All header guards must use #ifndef, not #pragma once
# ==========================================================================
echo "--- Checking NO_PRAGMA_ONCE ---"

pragma_hits=""
pragma_hits=$(grep -rn '#pragma once' "$ROOT/srcs/" "$ROOT/includes/" \
    --include='*.hpp' --include='*.h' 2>/dev/null || true)

if [ -n "$pragma_hits" ]; then
    printf '%s\n' "$pragma_hits" | while IFS= read -r hit; do
        _file=$(printf '%s' "$hit" | cut -d: -f1)
        _line=$(printf '%s' "$hit" | cut -d: -f2)
        echo "VIOLATION [NO_PRAGMA_ONCE] $_file:$_line — #pragma once (use #ifndef guards)"
    done
    _cnt=$(printf '%s\n' "$pragma_hits" | wc -l | tr -d ' ')
    violations=$((violations + _cnt))
    echo "FAIL: $_cnt #pragma once occurrence(s)"
else
    echo "PASS"
fi

# ==========================================================================
# 6. NO_MAKEFILE_GLOBS
#    Makefile must not use *.cpp, *.o, *.hpp, or $(wildcard ...)
# ==========================================================================
echo "--- Checking NO_MAKEFILE_GLOBS ---"

makefile_path="$ROOT/Makefile"
if [ -f "$makefile_path" ]; then
    glob_hits=""
    glob_hits=$(grep -n '\*\.cpp\|\*\.o\|\*\.hpp\|\$(wildcard' "$makefile_path" 2>/dev/null || true)

    if [ -n "$glob_hits" ]; then
        printf '%s\n' "$glob_hits" | while IFS= read -r hit; do
            _line=$(printf '%s' "$hit" | cut -d: -f1)
            echo "VIOLATION [NO_MAKEFILE_GLOBS] $makefile_path:$_line — glob pattern in Makefile"
        done
        _cnt=$(printf '%s\n' "$glob_hits" | wc -l | tr -d ' ')
        violations=$((violations + _cnt))
        echo "FAIL: $_cnt glob pattern(s) in Makefile"
    else
        echo "PASS"
    fi
else
    echo "SKIP: No Makefile found at $makefile_path"
fi

# ==========================================================================
# Summary
# ==========================================================================
echo ""
echo "========================================"
if [ "$violations" -gt 0 ]; then
    echo "RESULT: $violations violation(s) found"
    echo "========================================"
    exit 1
else
    echo "RESULT: All checks passed"
    echo "========================================"
    exit 0
fi
