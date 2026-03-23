// 05-cgi-error-handling.cpp
// Educational case: Error handling and edge cases in CGI
//
// Compile: g++ -std=c++98 -Wall -Wextra 05-cgi-error-handling.cpp -o 05-cgi-error-handling
// Run: ./05-cgi-error-handling
//
// This demonstrates:
// 1. Checking return values from system calls
// 2. Handling fork() failures
// 3. Handling exec() failures
// 4. Script timeout scenarios
// 5. Broken pipes and signal handling
// 6. Resource cleanup on errors

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <sstream>
#include <cerrno>

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

struct ScriptResult {
    bool success;
    std::string error_message;
    int exit_code;
    std::string output;
    
    ScriptResult() : success(false), exit_code(-1) {}
};

// ============================================================================
// ERROR HANDLING PATTERNS
// ============================================================================

// Pattern 1: Safe pipe creation
bool safe_pipe(int fd[2], std::string& error)
{
    if (pipe(fd) == -1) {
        error = "pipe() failed: ";
        error += strerror(errno);
        return false;
    }
    return true;
}

// Pattern 2: Safe fork with complete error checking
pid_t safe_fork(std::string& error)
{
    pid_t pid = fork();
    
    if (pid == -1) {
        error = "fork() failed: ";
        error += strerror(errno);
        return -1;
    }
    
    return pid;
}

// Pattern 3: Safe exec with error reporting
void safe_exec(const char* program, const char* const args[], std::string& error)
{
    execvp(program, (char* const*)args);
    
    // If we get here, exec failed
    error = "exec() failed: ";
    error += strerror(errno);
}

// Pattern 4: Safe read from pipe
bool safe_read(int fd, char* buffer, size_t size, ssize_t& bytes_read, std::string& error)
{
    bytes_read = read(fd, buffer, size);
    
    if (bytes_read == -1) {
        error = "read() failed: ";
        error += strerror(errno);
        return false;
    }
    
    return true;
}

// Pattern 5: Check script existence and permissions
bool check_script_executable(const std::string& path, std::string& error)
{
    struct stat st;
    
    if (stat(path.c_str(), &st) == -1) {
        error = "Script not found: " + path;
        return false;
    }
    
    if (!(st.st_mode & S_IFREG)) {
        error = "Not a regular file: " + path;
        return false;
    }
    
    if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
        error = "Script not executable: " + path;
        return false;
    }
    
    return true;
}

// Pattern 6: Exit code interpretation
std::string interpret_exit_code(int exit_code)
{
    std::string interpretation = "";
    
    switch (exit_code) {
        case 0:
            interpretation = "Success (0)";
            break;
        case 1:
            interpretation = "General error (1)";
            break;
        case 2:
            interpretation = "Misuse of shell command (2)";
            break;
        case 127:
            interpretation = "Command not found (127)";
            break;
        default:
            if (exit_code > 128) {
                int signal = exit_code - 128;
                interpretation = "Killed by signal " + size_to_string(signal) + " (" 
                               + (exit_code - 128) + ")";
            } else {
                interpretation = "Exit code: " + size_to_string(exit_code);
            }
    }
    
    return interpretation;
}

// Complete error-safe CGI executor
ScriptResult execute_cgi_safe(const std::string& script_path)
{
    ScriptResult result;
    
    // Step 1: Validate script
    if (!check_script_executable(script_path, result.error_message)) {
        return result;
    }
    
    // Step 2: Create pipes
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (!safe_pipe(stdin_pipe, result.error_message)) {
        return result;
    }
    
    if (!safe_pipe(stdout_pipe, result.error_message)) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        return result;
    }
    
    // Step 3: Fork
    pid_t pid = safe_fork(result.error_message);
    if (pid == -1) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return result;
    }
    
    if (pid == 0) {
        // ===== CHILD PROCESS =====
        
        // Redirect pipes
        if (dup2(stdin_pipe[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin");
            exit(127);
        }
        if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout");
            exit(127);
        }
        
        // Close all pipe file descriptors
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        
        // Set environment
        setenv("REQUEST_METHOD", "GET", 1);
        setenv("SCRIPT_NAME", script_path.c_str(), 1);
        setenv("QUERY_STRING", "", 1);
        
        // Execute
        const char* args[] = { script_path.c_str(), NULL };
        execv(script_path.c_str(), (char* const*)args);
        
        // If exec fails, exit with code 127
        perror("execv");
        exit(127);
    }
    else {
        // ===== PARENT PROCESS =====
        
        // Close unused ends
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Close write end to signal EOF
        close(stdin_pipe[1]);
        
        // Read output
        char buffer[4096];
        ssize_t bytes_read;
        std::string error;
        
        while (safe_read(stdout_pipe[0], buffer, sizeof(buffer), bytes_read, error)) {
            if (bytes_read == 0) break;  // EOF
            result.output.append(buffer, bytes_read);
        }
        
        close(stdout_pipe[0]);
        
        // Wait for child
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            result.error_message = "waitpid() failed: ";
            result.error_message += strerror(errno);
            return result;
        }
        
        // Interpret exit status
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
            if (result.exit_code != 0) {
                result.error_message = "Script exited with error code " 
                                      + size_to_string(result.exit_code);
                return result;
            }
        } else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            result.error_message = "Script killed by signal " + size_to_string(signal);
            return result;
        } else {
            result.error_message = "Unknown wait status";
            return result;
        }
        
        result.success = true;
        return result;
    }
    
    return result;  // Never reached
}

// ============================================================================
// TEST CASES
// ============================================================================

void case_1_script_not_found()
{
    print_separator("Case 1: Script Not Found");
    
    std::string error;
    bool exists = check_script_executable("/nonexistent/script.py", error);
    
    std::cout << "Checking: /nonexistent/script.py" << std::endl;
    std::cout << "Result: " << (exists ? "OK" : "ERROR") << std::endl;
    std::cout << "Message: " << error << std::endl;
}

void case_2_script_not_executable()
{
    print_separator("Case 2: Script Not Executable");
    
    // Create a non-executable file
    const char* test_file = "/tmp/non_executable.py";
    FILE* f = fopen(test_file, "w");
    fprintf(f, "#!/usr/bin/env python\nprint 'hello'\n");
    fclose(f);
    
    // Don't set execute permission
    chmod(test_file, 0644);
    
    std::string error;
    bool exists = check_script_executable(test_file, error);
    
    std::cout << "Created: " << test_file << " (without execute permission)" << std::endl;
    std::cout << "Result: " << (exists ? "OK" : "ERROR") << std::endl;
    std::cout << "Message: " << error << std::endl;
    
    // Cleanup
    unlink(test_file);
}

void case_3_pipe_failure()
{
    print_separator("Case 3: Pipe Creation Failure Handling");
    
    std::cout << "Simulating: Successfully checking for pipe() failures" << std::endl;
    std::cout << "(In production, pipe() rarely fails unless system is out of FDs)" << std::endl;
    
    std::string error;
    int fd[2];
    bool success = safe_pipe(fd, error);
    
    std::cout << "Result: " << (success ? "OK" : "FAILED") << std::endl;
    if (success) {
        std::cout << "Created pipes [" << fd[0] << ", " << fd[1] << "]" << std::endl;
        close(fd[0]);
        close(fd[1]);
    } else {
        std::cout << "Error: " << error << std::endl;
    }
}

void case_4_exit_code_interpretation()
{
    print_separator("Case 4: Exit Code Interpretation");
    
    std::cout << "Common exit codes:" << std::endl;
    
    int test_codes[] = { 0, 1, 2, 127, 255 };
    
    for (int i = 0; i < 5; ++i) {
        std::cout << "  " << test_codes[i] << " → " 
                  << interpret_exit_code(test_codes[i]) << std::endl;
    }
}

void case_5_resource_cleanup()
{
    print_separator("Case 5: Safe Resource Cleanup on Error");
    
    std::cout << "Pattern: Close all resources even when error occurs" << std::endl;
    std::cout << "\nCode pattern:" << std::endl;
    std::cout << R"(
    // Create first pipe
    if (pipe(stdin_pipe) == -1) {
        error_handler();
        return;  // NO cleanup needed yet
    }
    
    // Create second pipe — if this fails, cleanup first!
    if (pipe(stdout_pipe) == -1) {
        close(stdin_pipe[0]);   // ← CLEAN UP FIRST PIPE
        close(stdin_pipe[1]);
        error_handler();
        return;
    }
    
    // If fork fails, cleanup both pipes
    pid = fork();
    if (pid == -1) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        error_handler();
        return;
    }
    )" << std::endl;
    
    std::cout << "Key: Order matters! Close in reverse of allocation order." << std::endl;
}

void case_6_common_mistakes()
{
    print_separator("Case 6: Common Error Handling Mistakes");
    
    std::cout << "❌ WRONG:" << std::endl;
    std::cout << "  if (fork() > 0)" << std::endl;
    std::cout << "     // assumes pid > 0 means parent" << std::endl;
    std::cout << "     // if fork fails, pid = -1, treated as true!" << std::endl;
    std::cout << std::endl;
    
    std::cout << "✅ CORRECT:" << std::endl;
    std::cout << "  pid = fork();" << std::endl;
    std::cout << "  if (pid == -1)" << std::endl;
    std::cout << "      error();" << std::endl;
    std::cout << "  if (pid == 0)" << std::endl;
    std::cout << "      child();" << std::endl;
    std::cout << "  else  // pid > 0" << std::endl;
    std::cout << "      parent();" << std::endl;
    std::cout << std::endl;
    
    std::cout << "❌ WRONG:" << std::endl;
    std::cout << "  execv(script, args);" << std::endl;
    std::cout << "  // assumes exec succeeds; code continues if it fails!" << std::endl;
    std::cout << std::endl;
    
    std::cout << "✅ CORRECT:" << std::endl;
    std::cout << "  execv(script, args);" << std::endl;
    std::cout << "  // exec never returns on success, so this is error" << std::endl;
    std::cout << "  perror(\"execv\");" << std::endl;
    std::cout << "  exit(127);  // ← EXIT IMMEDIATELY" << std::endl;
    std::cout << std::endl;
    
    std::cout << "❌ WRONG:" << std::endl;
    std::cout << "  read(pipe, buffer, size);  // ignore return value" << std::endl;
    std::cout << std::endl;
    
    std::cout << "✅ CORRECT:" << std::endl;
    std::cout << "  ssize_t bytes = read(pipe, buffer, size);" << std::endl;
    std::cout << "  if (bytes == -1)" << std::endl;
    std::cout << "      perror(\"read\");  // error" << std::endl;
    std::cout << "  if (bytes == 0)" << std::endl;
    std::cout << "      break;  // EOF" << std::endl;
    std::cout << std::endl;
}

void case_7_safe_execution()
{
    print_separator("Case 7: Safe Script Execution (Complete Example)");
    
    std::cout << "Creating test script..." << std::endl;
    
    // Create a simple executable script
    const char* test_script = "/tmp/test_cgi_safe.sh";
    FILE* f = fopen(test_script, "w");
    fprintf(f, "#!/bin/bash\necho 'Script output: Hello from safe execution'\n");
    fclose(f);
    chmod(test_script, 0755);
    
    std::cout << "Executing: " << test_script << std::endl;
    
    ScriptResult result = execute_cgi_safe(test_script);
    
    std::cout << "\nResult:" << std::endl;
    std::cout << "  Success: " << (result.success ? "YES" : "NO") << std::endl;
    std::cout << "  Exit Code: " << result.exit_code << std::endl;
    
    if (!result.error_message.empty()) {
        std::cout << "  Error: " << result.error_message << std::endl;
    }
    
    if (!result.output.empty()) {
        std::cout << "  Output: " << result.output << std::endl;
    }
    
    // Cleanup
    unlink(test_script);
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  CGI Error Handling & Edge Cases                  █" << std::endl;
    std::cout << "█  Educational Test Cases                           █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    case_1_script_not_found();
    case_2_script_not_executable();
    case_3_pipe_failure();
    case_4_exit_code_interpretation();
    case_5_resource_cleanup();
    case_6_common_mistakes();
    case_7_safe_execution();
    
    print_separator("All cases completed!");
    std::cout << "\nThe golden rule: Always check return values and cleanup resources." << std::endl;
    
    return 0;
}
