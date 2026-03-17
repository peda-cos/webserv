// 06-cgi-advanced-patterns.cpp
// Educational case: Advanced CGI patterns and performance
//
// Compile: g++ -std=c++98 -Wall -Wextra 06-cgi-advanced-patterns.cpp -o 06-cgi-advanced-patterns
// Run: ./06-cgi-advanced-patterns
//
// This demonstrates:
// 1. Non-blocking I/O with select() and poll()
// 2. Script timeouts
// 3. Handling large POST bodies
// 4. Preventing buffer overflow
// 5. Performance considerations (forking vs threading, etc.)
// 6. Security aspects (file path validation, etc.)

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <sstream>
#include <algorithm>

void print_separator(const std::string& title)
{
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
}

std::string size_to_string(size_t value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// ============================================================================
// PATTERN 1: Non-blocking I/O with select()
// ============================================================================

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Pattern: Read from pipe with timeout using select()
std::string read_with_timeout(int fd, int timeout_seconds)
{
    std::string result = "";
    char buffer[4096];
    
    // Set non-blocking mode
    set_nonblocking(fd);
    
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        struct timeval tv;
        tv.tv_sec = timeout_seconds;
        tv.tv_usec = 0;
        
        int select_ret = select(fd + 1, &readfds, NULL, NULL, &tv);
        
        if (select_ret == -1) {
            perror("select");
            break;
        } else if (select_ret == 0) {
            // Timeout occurred
            result += "[TIMEOUT - waiting too long]";
            break;
        } else if (FD_ISSET(fd, &readfds)) {
            // Data available
            ssize_t bytes = read(fd, buffer, sizeof(buffer));
            
            if (bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data now, but select said there was (shouldn't happen)
                    continue;
                }
                perror("read");
                break;
            } else if (bytes == 0) {
                // EOF
                break;
            } else {
                result.append(buffer, bytes);
            }
        }
    }
    
    return result;
}

// Pattern: Set file descriptor to non-blocking
void make_nonblocking(int fd)
{
    set_nonblocking(fd);
}

// ============================================================================
// PATTERN 2: Script Timeout Handling
// ============================================================================

// Execute script with timeout
bool execute_cgi_with_timeout(
    const std::string& script_path,
    int timeout_seconds,
    std::string& output,
    std::string& error_msg
)
{
    // Note: This is a simplified pattern. 
    // Real timeout requires more careful signal handling or SIGALRM.
    
    int pipes[2];
    if (pipe(pipes) == -1) {
        error_msg = "pipe() failed";
        return false;
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[1]);
        
        execl(script_path.c_str(), script_path.c_str(), NULL);
        exit(127);
    } else if (pid > 0) {
        // Parent
        close(pipes[1]);
        
        output = read_with_timeout(pipes[0], timeout_seconds);
        close(pipes[0]);
        
        // Check if child finished
        int status;
        pid_t wait_ret = waitpid(pid, &status, WNOHANG);  // Non-blocking wait
        
        if (wait_ret == 0) {
            // Child still running - timeout scenario
            // In production, would kill with SIGTERM
            kill(pid, SIGTERM);
            usleep(100000);  // Give it 0.1 sec to cleanup
            
            if (waitpid(pid, &status, WNOHANG) == 0) {
                // Still running, force kill
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }
            
            error_msg = "Script timeout after " + size_to_string(timeout_seconds) + "s";
            return false;
        }
        
        return true;
    } else {
        error_msg = "fork() failed";
        return false;
    }
}

// ============================================================================
// PATTERN 3: Large POST Body Handling
// ============================================================================

// Chunk size for reading large bodies
const size_t CHUNK_SIZE = 8192;
const size_t MAX_BODY_SIZE = 104857600;  // 100MB limit

bool write_to_child_chunked(
    int fd,
    const std::string& body,
    std::string& error_msg
)
{
    size_t total = body.size();
    size_t written = 0;
    
    std::cout << "Writing " << total << " bytes in chunks of " 
              << CHUNK_SIZE << "..." << std::endl;
    
    while (written < total) {
        size_t to_write = std::min(CHUNK_SIZE, total - written);
        
        ssize_t bytes = write(fd, body.c_str() + written, to_write);
        
        if (bytes == -1) {
            error_msg = "write() failed";
            return false;
        }
        
        written += bytes;
        
        // Show progress
        int percent = (written * 100) / total;
        std::cout << "  [" << percent << "%] " << written << "/" << total << std::endl;
    }
    
    std::cout << "Write complete." << std::endl;
    return true;
}

// ============================================================================
// PATTERN 4: Buffer Overflow Prevention
// ============================================================================

// Safe string copy with bounds checking
bool safe_strcpy(char* dest, size_t dest_size, const char* src)
{
    size_t src_len = strlen(src);
    
    if (src_len >= dest_size) {
        return false;  // Would overflow
    }
    
    strcpy(dest, src);
    return true;
}

// Safe read with buffer limit
std::string safe_read_limited(int fd, size_t max_bytes)
{
    std::string result = "";
    char buffer[4096];
    size_t total = 0;
    
    while (total < max_bytes) {
        size_t to_read = std::min(sizeof(buffer), max_bytes - total);
        ssize_t bytes = read(fd, buffer, to_read);
        
        if (bytes == -1) {
            perror("read");
            break;
        }
        if (bytes == 0) break;
        
        result.append(buffer, bytes);
        total += bytes;
    }
    
    return result;
}

// ============================================================================
// PATTERN 5: Security - Path Validation
// ============================================================================

// Prevent directory traversal attacks
bool is_safe_path(const std::string& path, const std::string& base_dir)
{
    (void)base_dir; // Avoid unused parameter warning

    // Check for ".." in path
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    // Check for absolute paths
    if (path[0] == '/') {
        return false;  // Only relative paths allowed
    }
    
    // Could add more checks:
    // - symlink validation
    // - chroot restrictions
    // - etc.
    
    return true;
}

// ============================================================================
// PATTERN 6: Performance Considerations
// ============================================================================

void discuss_performance()
{
    print_separator("Performance Considerations");
    
    std::cout << "1. FORK MODEL:" << std::endl;
    std::cout << "   Pros:" << std::endl;
    std::cout << "   └─ Each script has isolated memory/FDs" << std::endl;
    std::cout << "   └─ One bad script can't crash others" << std::endl;
    std::cout << "   └─ Simple model, easy to understand" << std::endl;
    std::cout << std::endl;
    
    std::cout << "   Cons:" << std::endl;
    std::cout << "   └─ fork() is expensive (copy all memory)" << std::endl;
    std::cout << "   └─ Can't handle thousands of concurrent requests" << std::endl;
    std::cout << "   └─ Each request = at least one fork()" << std::endl;
    std::cout << std::endl;
    
    std::cout << "2. OPTIMIZATION STRATEGIES:" << std::endl;
    std::cout << "   └─ Thread pools (keep interpreters ready)" << std::endl;
    std::cout << "   └─ FastCGI (persistent script process)" << std::endl;
    std::cout << "   └─ Caching (memoize results)" << std::endl;
    std::cout << "   └─ Pre-compile (PHP bytecode cache)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "3. FOR THIS PROJECT:" << std::endl;
    std::cout << "   └─ MVP: Simple fork() for each CGI request" << std::endl;
    std::cout << "   └─ Acceptable for ~10-100 concurrent requests" << std::endl;
    std::cout << "   └─ Beyond that, use FastCGI or application server" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// TEST CASES
// ============================================================================

void case_1_nonblocking_read()
{
    print_separator("Case 1: Non-blocking Read with select()");
    
    std::cout << "Pattern: Use select() to wait for data with timeout" << std::endl;
    std::cout << "\nAdvantage: Can handle multiple FDs simultaneously" << std::endl;
    std::cout << "In webserver: Read from multiple clients at once" << std::endl;
    
    std::cout << "\nCode pattern:" << std::endl;
    std::cout << "\n"
    "    fd_set readfds;\n"
    "    FD_ZERO(&readfds);\n"
    "    FD_SET(pipe_fd, &readfds);\n"
    "    \n"
    "    struct timeval tv;\n"
    "    tv.tv_sec = timeout_seconds;\n"
    "    tv.tv_usec = 0;\n"
    "    \n"
    "    int ret = select(pipe_fd + 1, &readfds, NULL, NULL, &tv);\n"
    "    \n"
    "    if (ret == 0) {\n"
    "        // Timeout\n"
    "    } else if (FD_ISSET(pipe_fd, &readfds)) {\n"
    "        // Data available, safe to read()\n"
    "        read(pipe_fd, buffer, size);\n"
    "    }\n"
    "    " << std::endl;
}

void case_2_timeout_handling()
{
    print_separator("Case 2: Script Timeout (Conceptual)");
    
    std::cout << "Scenario: Script takes too long to finish" << std::endl;
    std::cout << "(e.g., infinite loop, slow database query)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Solution: Use SIGALRM + select() + waitpid(WNOHANG)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "timeout_t = fork() + exec(script)" << std::endl;
    std::cout << "      ↓ (set alarm for N seconds)" << std::endl;
    std::cout << "   select(timeout=N) on pipe" << std::endl;
    std::cout << "      ↓ (if timeout expired)" << std::endl;
    std::cout << "   kill(pid, SIGTERM)" << std::endl;
    std::cout << "      ↓ (give it time to cleanup)" << std::endl;
    std::cout << "   waitpid(WNOHANG) → still running?" << std::endl;
    std::cout << "      ↓ (if yes, force kill)" << std::endl;
    std::cout << "   kill(pid, SIGKILL)" << std::endl;
    std::cout << "      ↓" << std::endl;
    std::cout << "   waitpid() → wait for death" << std::endl;
    std::cout << "      ↓" << std::endl;
    std::cout << "   Return: \"504 Gateway Timeout\"" << std::endl;
}

void case_3_large_body()
{
    print_separator("Case 3: Large POST Body Handling");
    
    std::cout << "Scenario: Client uploads 50MB file" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Problem: Can't read into buffer at once" << std::endl;
    std::cout << "Solution: Read/write in chunks" << std::endl;
    std::cout << std::endl;
    
    // Simulate 1MB body
    std::cout << "Simulating 1MB body write..." << std::endl;
    
    std::string large_body(1048576, 'x');  // 1MB of 'x'
    
    int pipes[2];
    pipe(pipes);
    
    pid_t pid = fork();
    if (pid == 0) {
        // Child process to constantly read from the pipe
        // This prevents the pipe buffer from filling up and blocking write()
        close(pipes[1]);
        char buf[8192];
        while (read(pipes[0], buf, sizeof(buf)) > 0) {
            // Drain data
        }
        close(pipes[0]);
        exit(0);
    }
    
    close(pipes[0]); // Parent closes read end
    
    std::string error_msg;
    if (write_to_child_chunked(pipes[1], large_body, error_msg)) {
        std::cout << "Success!" << std::endl;
    } else {
        std::cout << "Error: " << error_msg << std::endl;
    }
    
    close(pipes[1]);
    waitpid(pid, NULL, 0); // Wait for child to exit
}

void case_4_buffer_safety()
{
    print_separator("Case 4: Buffer Overflow Prevention");
    
    std::cout << "Pattern: Always bounds-check before copying" << std::endl;
    std::cout << std::endl;
    
    std::cout << "❌ UNSAFE:" << std::endl;
    std::cout << "  strcpy(buffer, user_input);  // Can overflow!" << std::endl;
    std::cout << std::endl;
    
    std::cout << "✅ SAFE:" << std::endl;
    std::cout << "  strncpy(buffer, user_input, sizeof(buffer) - 1);" << std::endl;
    std::cout << "  buffer[sizeof(buffer) - 1] = '\\0';" << std::endl;
    std::cout << "  // Now safe, null-terminated, truncated if needed" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Or use safer function:" << std::endl;
    std::cout << "  if (!safe_strcpy(buffer, sizeof(buffer), user_input)) {" << std::endl;
    std::cout << "      error(\"String too long\");" << std::endl;
    std::cout << "  }" << std::endl;
}

void case_5_path_security()
{
    print_separator("Case 5: Path Security Checks");
    
    std::cout << "Prevent path traversal attacks:" << std::endl;
    std::cout << std::endl;
    
    struct TestPath {
        std::string path;
        bool safe;
    };
    
    TestPath tests[] = {
        { "cgi-bin/script.py", true },
        { "cgi-bin/../../../etc/passwd", false },  // Has ".."
        { "/etc/passwd", false },                  // Absolute path
        { "api/users.php", true },
        { "admin/../../etc/shadow", false },
    };
    
    std::cout << "Testing paths:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        bool safe = is_safe_path(tests[i].path, "/var/www");
        std::string expected = tests[i].safe ? "✓" : "✗";
        std::string actual = safe ? "✓" : "✗";
        
        std::cout << "  \"" << tests[i].path << "\" → " << actual;
        if (actual == expected) {
            std::cout << " PASS" << std::endl;
        } else {
            std::cout << " FAIL" << std::endl;
        }
    }
}

void case_6_performance_discussion()
{
    discuss_performance();
}

void case_7_real_world_architecture()
{
    print_separator("Case 7: Real-World Architectures");
    
    std::cout << "Nginx (reference):" << std::endl;
    std::cout << "  ├─ Main process (runs as root)" << std::endl;
    std::cout << "  ├─ Worker processes (N processes)" << std::endl;
    std::cout << "  │  └─ Each handles multiple connections" << std::endl;
    std::cout << "  │  └─ Non-blocking I/O with epoll/kqueue" << std::endl;
    std::cout << "  └─ For CGI: forks ONE process per request" << std::endl;
    std::cout << std::endl;
    
    std::cout << "FastCGI (common optimization):" << std::endl;
    std::cout << "  ├─ Separate FastCGI server (e.g., PHP-FPM)" << std::endl;
    std::cout << "  ├─ Nginx connects via socket" << std::endl;
    std::cout << "  ├─ PHP process pool ready to process" << std::endl;
    std::cout << "  └─ No fork() needed for each request" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Project (MVP):" << std::endl;
    std::cout << "  ├─ Single thread, non-blocking I/O with poll()" << std::endl;
    std::cout << "  ├─ For each CGI request:" << std::endl;
    std::cout << "  │  ├─ fork()" << std::endl;
    std::cout << "  │  ├─ exec() script" << std::endl;
    std::cout << "  │  └─ waitpid()" << std::endl;
    std::cout << "  └─ Suitable for 10-100 concurrent requests" << std::endl;
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  Advanced CGI Patterns & Performance              █" << std::endl;
    std::cout << "█  Educational Cases & Concepts                    █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    case_1_nonblocking_read();
    case_2_timeout_handling();
    case_3_large_body();
    case_4_buffer_safety();
    case_5_path_security();
    case_6_performance_discussion();
    case_7_real_world_architecture();
    
    print_separator("All cases completed!");
    std::cout << "\nCore Principle:" << std::endl;
    std::cout << "fork() is simple but expensive. For real scale, use async patterns" << std::endl;
    std::cout << "or delegate to specialized CGI servers (FastCGI, WSGI, etc.)" << std::endl;
    
    return 0;
}
