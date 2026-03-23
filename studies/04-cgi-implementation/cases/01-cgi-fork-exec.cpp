// 01-cgi-fork-exec.cpp
// Educational case: Basic fork() + exec() pattern for CGI
// 
// Compile: g++ -std=c++98 -Wall -Wextra 01-cgi-fork-exec.cpp -o 01-cgi-fork-exec
// Run: ./01-cgi-fork-exec
//
// This demonstrates:
// 1. Creating a child process with fork()
// 2. Redirecting stdin/stdout with dup2()
// 3. Executing external program with exec()
// 4. Capturing output with pipes
// 5. Waiting for child with waitpid()

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <errno.h>

void print_separator(const std::string& title)
{
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
}

// Case 1: Simple fork() and exec() to run /bin/echo
void case_1_simple_exec()
{
    print_separator("Case 1: Simple exec() — Run /bin/echo");
    
    std::cout << "Parent: About to fork..." << std::endl;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return;
    }
    
    if (pid == 0) {
        // ===== CHILD =====
        std::cout << "Child: I am PID " << getpid() << std::endl;
        
        // Execute /bin/echo
        const char* args[] = { "echo", "Hello from child process!", NULL };
        execvp("echo", (char* const*)args);  // execvp searches PATH
        
        // If exec succeeds, this code NEVER RUNS
        std::cerr << "Child: exec failed!" << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT =====
        std::cout << "Parent: Child PID is " << pid << std::endl;
        
        int status;
        waitpid(pid, &status, 0);  // Wait for child to finish
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            std::cout << "Parent: Child exited with code " << exit_code << std::endl;
        }
    }
}

// Case 2: fork() with pipe for input
void case_2_pipe_input()
{
    print_separator("Case 2: Pipe for Input — Sort data via child");
    
    // Create pipe for input
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Pipe failed: " << strerror(errno) << std::endl;
        return;
    }
    
    std::cout << "Parent: Created pipe. FDs: [" << pipe_fd[0] 
              << " (read), " << pipe_fd[1] << " (write)]" << std::endl;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return;
    }
    
    if (pid == 0) {
        // ===== CHILD =====
        std::cout << "Child: Connecting stdin to pipe[0]..." << std::endl;
        
        // Redirect pipe[0] to stdin (fd 0)
        dup2(pipe_fd[0], STDIN_FILENO);
        
        // Close both ends (we've redefined them)
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        
        // Execute sort command (reads from stdin)
        const char* args[] = { "sort", NULL };
        execvp("sort", (char* const*)args);
        
        std::cerr << "Child: exec failed!" << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT =====
        std::cout << "Parent: Writing data to pipe..." << std::endl;
        
        // Close read end (we won't read)
        close(pipe_fd[0]);
        
        // Write unsorted data to pipe
        const char* data = "zebra\napple\nmango\nbanana\n";
        ssize_t written = write(pipe_fd[1], data, strlen(data));
        
        std::cout << "Parent: Wrote " << written << " bytes" << std::endl;
        
        // Close write end to signal EOF
        close(pipe_fd[1]);
        
        std::cout << "Parent: Waiting for child to finish..." << std::endl;
        
        int status;
        waitpid(pid, &status, 0);
        
        std::cout << "Parent: Child finished. (Output above is sorted)" << std::endl;
    }
}

// Case 3: fork() with pipe for output
void case_3_pipe_output()
{
    print_separator("Case 3: Pipe for Output — Capture child's stdout");
    
    // Create pipe for output
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        std::cerr << "Pipe failed: " << strerror(errno) << std::endl;
        return;
    }
    
    std::cout << "Parent: Created pipe for capturing output" << std::endl;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return;
    }
    
    if (pid == 0) {
        // ===== CHILD =====
        
        // Redirect pipe[1] to stdout (fd 1)
        dup2(pipe_fd[1], STDOUT_FILENO);
        
        // Close both ends
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        
        // Execute "date" command (writes to stdout)
        const char* args[] = { "date", NULL };
        execvp("date", (char* const*)args);
        
        std::cerr << "Child: exec failed!" << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT =====
        
        // Close write end (we won't write)
        close(pipe_fd[1]);
        
        // Read output from child
        char buffer[512];
        std::string output = "";
        
        std::cout << "Parent: Reading child's output..." << std::endl;
        
        while (true) {
            ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer));
            
            if (bytes_read == -1) {
                std::cerr << "Read failed: " << strerror(errno) << std::endl;
                break;
            }
            if (bytes_read == 0) {
                break;  // EOF
            }
            
            output.append(buffer, bytes_read);
        }
        
        close(pipe_fd[0]);
        
        std::cout << "Parent: Captured " << output.size() << " bytes from child:" << std::endl;
        std::cout << "---OUTPUT---" << std::endl;
        std::cout << output;
        std::cout << "---END---" << std::endl;
        
        int status;
        waitpid(pid, &status, 0);
        std::cout << "Parent: Child finished" << std::endl;
    }
}

// Case 4: Bidirectional pipes (stdin AND stdout)
void case_4_bidirectional()
{
    print_separator("Case 4: Bidirectional Pipes — Send data AND capture output");
    
    // Create two pipes
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        std::cerr << "Pipe failed: " << strerror(errno) << std::endl;
        return;
    }
    
    std::cout << "Parent: Created two pipes:" << std::endl;
    std::cout << "  stdin_pipe  for child input" << std::endl;
    std::cout << "  stdout_pipe for child output" << std::endl;
    
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return;
    }
    
    if (pid == 0) {
        // ===== CHILD =====
        
        // Redirect pipes to stdin and stdout
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        // Close all original pipe FDs
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        
        // Execute "tr" command (translate/transform from stdin)
        // tr '[:lower:]' '[:upper:]' converts lowercase to uppercase
        const char* args[] = { "tr", "a-z", "A-Z", NULL };
        execvp("tr", (char* const*)args);
        
        std::cerr << "Child: exec failed!" << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT =====
        
        // Close unused ends
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Write data to child
        const char* input_data = "hello world from parent\n";
        std::cout << "Parent: Sending to child: \"" << input_data << "\"" << std::endl;
        
        ssize_t written = write(stdin_pipe[1], input_data, strlen(input_data));
        std::cout << "Parent: Wrote " << written << " bytes" << std::endl;
        
        // Close write end (signal EOF to child)
        close(stdin_pipe[1]);
        
        // Read output from child
        char buffer[512];
        std::string output = "";
        
        std::cout << "Parent: Reading child's output..." << std::endl;
        
        while (true) {
            ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
            if (bytes_read <= 0) break;
            output.append(buffer, bytes_read);
        }
        
        close(stdout_pipe[0]);
        
        std::cout << "Parent: Received from child: \"" << output << "\"" << std::endl;
        
        int status;
        waitpid(pid, &status, 0);
        std::cout << "Parent: Child finished" << std::endl;
    }
}

// Case 5: Checking return status with WIFEXITED and WEXITSTATUS
void case_5_exit_codes()
{
    print_separator("Case 5: Exit Codes — Check child's return value");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // ===== CHILD =====
        std::cout << "Child: Exiting with code 42" << std::endl;
        exit(42);  // Custom exit code
    }
    else {
        // ===== PARENT =====
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            std::cout << "Parent: Child exited normally with code " << exit_code << std::endl;
        }
        else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::cout << "Parent: Child terminated by signal " << signal << std::endl;
        }
    }
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  fork() + exec() + pipes — CGI Foundation         █" << std::endl;
    std::cout << "█  Educational Test Cases                           █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    case_1_simple_exec();       // Basic fork and exec
    case_2_pipe_input();        // Child reading from parent
    case_3_pipe_output();       // Parent reading from child
    case_4_bidirectional();     // Bidirectional communication
    case_5_exit_codes();        // Checking exit codes
    
    print_separator("All cases completed!");
    
    return 0;
}
