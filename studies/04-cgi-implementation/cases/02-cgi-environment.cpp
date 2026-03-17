// 02-cgi-environment.cpp
// Educational case: Setting environment variables for CGI scripts
//
// Compile: g++ -std=c++98 -Wall -Wextra 02-cgi-environment.cpp -o 02-cgi-environment
// Run: ./02-cgi-environment
//
// This demonstrates:
// 1. Using setenv() to define environment variables
// 2. How child processes inherit parent environment
// 3. Setting CGI-specific variables
// 4. Verifying environment with getenv()

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <string>
#include <cerrno>

void print_separator(const std::string& title)
{
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
}

// Helper: Convert size_t to string
std::string size_to_string(size_t value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// Case 1: Reading environment variables
void case_1_read_environment()
{
    print_separator("Case 1: Read Environment Variables");
    
    // Some variables should already exist
    const char* path = getenv("PATH");
    const char* home = getenv("HOME");
    
    std::cout << "Parent: Reading existing environment variables:" << std::endl;
    std::cout << "  PATH = " << (path ? path : "(not set)") << std::endl;
    std::cout << "  HOME = " << (home ? home : "(not set)") << std::endl;
}

// Case 2: Setting new variables with setenv()
void case_2_setenv_basic()
{
    print_separator("Case 2: Setting Variables with setenv()");
    
    // setenv(name, value, overwrite)
    // If overwrite=1, replace if already exists
    // If overwrite=0, keep original if exists
    
    setenv("MY_VARIABLE", "Hello from webserv", 1);
    setenv("REQUEST_METHOD", "GET", 1);
    setenv("SCRIPT_NAME", "/cgi-bin/test.py", 1);
    
    std::cout << "Parent: Set three variables:" << std::endl;
    std::cout << "  MY_VARIABLE = " << getenv("MY_VARIABLE") << std::endl;
    std::cout << "  REQUEST_METHOD = " << getenv("REQUEST_METHOD") << std::endl;
    std::cout << "  SCRIPT_NAME = " << getenv("SCRIPT_NAME") << std::endl;
}

// Case 3: Child inherits parent environment
void case_3_inheritance()
{
    print_separator("Case 3: Environment Inheritance");
    
    // Set variable in parent
    setenv("PARENT_VAR", "I am from parent", 1);
    std::cout << "Parent: Set PARENT_VAR = \"I am from parent\"" << std::endl;
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // ===== CHILD =====
        
        const char* parent_var = getenv("PARENT_VAR");
        std::cout << "Child: Inherited PARENT_VAR = \"" 
                  << (parent_var ? parent_var : "(not found)") << "\"" << std::endl;
        
        // Modify variable in child (only affects child, not parent)
        setenv("PARENT_VAR", "Modified in child", 1);
        std::cout << "Child: Modified PARENT_VAR = \"" 
                  << getenv("PARENT_VAR") << "\"" << std::endl;
        
        exit(0);
    }
    else {
        // ===== PARENT =====
        int status;
        waitpid(pid, &status, 0);
        
        // Parent's value unchanged
        std::cout << "Parent: After child finished, PARENT_VAR = \"" 
                  << getenv("PARENT_VAR") << "\"" << std::endl;
    }
}

// Case 4: CGI environment setup simulation
void case_4_cgi_setup()
{
    print_separator("Case 4: Simulated CGI Environment Setup");
    
    // Simulate HTTP request
    std::string method = "GET";
    std::string script_name = "/cgi-bin/calculator.py";
    std::string query_string = "a=5&b=3&op=add";
    std::string server_name = "localhost";
    std::string server_port = "8080";
    size_t content_length = 0;  // GET has no body
    
    std::cout << "Parent: Simulating CGI request..." << std::endl;
    std::cout << "  REQUEST: " << method << " " << script_name 
              << "?" << query_string << std::endl;
    
    // Set CGI variables (always required)
    setenv("REQUEST_METHOD", method.c_str(), 1);
    setenv("SCRIPT_NAME", script_name.c_str(), 1);
    setenv("QUERY_STRING", query_string.c_str(), 1);
    setenv("SERVER_NAME", server_name.c_str(), 1);
    setenv("SERVER_PORT", server_port.c_str(), 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("REMOTE_ADDR", "192.168.1.50", 1);
    
    // For GET, content_length is 0
    std::string content_len_str = size_to_string(content_length);
    setenv("CONTENT_LENGTH", content_len_str.c_str(), 1);
    
    std::cout << "Parent: Set CGI environment variables:" << std::endl;
    std::cout << "  REQUEST_METHOD = " << getenv("REQUEST_METHOD") << std::endl;
    std::cout << "  SCRIPT_NAME = " << getenv("SCRIPT_NAME") << std::endl;
    std::cout << "  QUERY_STRING = " << getenv("QUERY_STRING") << std::endl;
    std::cout << "  SERVER_NAME = " << getenv("SERVER_NAME") << std::endl;
    std::cout << "  SERVER_PORT = " << getenv("SERVER_PORT") << std::endl;
}

// Case 5: POST request environment with content
void case_5_post_environment()
{
    print_separator("Case 5: POST Request Environment");
    
    // Simulate POST request
    std::string method = "POST";
    std::string script_name = "/cgi-bin/login.php";
    std::string post_data = "username=admin&password=secret123";
    size_t content_length = post_data.size();
    
    std::cout << "Parent: Simulating POST request..." << std::endl;
    std::cout << "  METHOD: " << method << std::endl;
    std::cout << "  SCRIPT: " << script_name << std::endl;
    std::cout << "  BODY: " << post_data << std::endl;
    std::cout << "  BODY SIZE: " << content_length << " bytes" << std::endl;
    
    // Set variables specific to POST
    setenv("REQUEST_METHOD", method.c_str(), 1);
    setenv("SCRIPT_NAME", script_name.c_str(), 1);
    setenv("QUERY_STRING", "", 1);  // POST doesn't use URL query
    setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
    
    std::string content_len_str = size_to_string(content_length);
    setenv("CONTENT_LENGTH", content_len_str.c_str(), 1);
    
    std::cout << "Parent: Set POST-specific variables:" << std::endl;
    std::cout << "  REQUEST_METHOD = " << getenv("REQUEST_METHOD") << std::endl;
    std::cout << "  CONTENT_TYPE = " << getenv("CONTENT_TYPE") << std::endl;
    std::cout << "  CONTENT_LENGTH = " << getenv("CONTENT_LENGTH") << std::endl;
    std::cout << "  QUERY_STRING = \"" << getenv("QUERY_STRING") << "\"" << std::endl;
}

// Case 6: Execute script with environment
void case_6_execute_with_env()
{
    print_separator("Case 6: Execute Script with Custom Environment");
    
    // Setup environment before fork
    setenv("MY_CGI_VAR", "Test123", 1);
    setenv("REQUEST_METHOD", "GET", 1);
    setenv("QUERY_STRING", "param=value", 1);
    
    std::cout << "Parent: Setup environment, about to fork..." << std::endl;
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // ===== CHILD =====
        
        // Child inherits all of parent's environment
        
        // Execute a shell command that prints environment
        // Using 'env' command to show what we're working with
        const char* args[] = { "env", "grep", "MY_CGI\\|REQUEST\\|QUERY", NULL };
        execvp("bash", (char* const*)args);
        
        // Alternative: Execute and grep for our variables
        // Each variable set will be visible
        
        // If you want to use Python instead:
        // const char* py_args[] = { "python", "-c", 
        //      "import os; print(os.environ.get('MY_CGI_VAR', 'NOT FOUND'))", 
        //      NULL };
        // execvp("python", (char* const*)py_args);
        
        std::cerr << "Exec failed: " << strerror(errno) << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT =====
        int status;
        waitpid(pid, &status, 0);
        
        std::cout << "Parent: Child finished" << std::endl;
    }
}

// Case 7: Environment variable overwrite behavior
void case_7_overwrite_behavior()
{
    print_separator("Case 7: Setenv Overwrite Behavior");
    
    // Set a variable
    setenv("CONFIG_VAR", "original_value", 1);
    std::cout << "Parent: Set CONFIG_VAR = \"original_value\"" << std::endl;
    
    // Try to set same variable with overwrite=0 (don't overwrite)
    int ret = setenv("CONFIG_VAR", "new_value", 0);  // 0 = don't overwrite
    if (ret == -1) {
        std::cerr << "setenv failed: " << strerror(errno) << std::endl;
    }
    
    std::cout << "Parent: Tried to setenv with overwrite=0..." << std::endl;
    std::cout << "  CONFIG_VAR = \"" << getenv("CONFIG_VAR") << "\"" << std::endl;
    std::cout << "  (Should still be original_value)" << std::endl;
    
    // Now with overwrite=1
    setenv("CONFIG_VAR", "final_value", 1);  // 1 = overwrite
    
    std::cout << "Parent: After setenv with overwrite=1..." << std::endl;
    std::cout << "  CONFIG_VAR = \"" << getenv("CONFIG_VAR") << "\"" << std::endl;
    std::cout << "  (Should be final_value)" << std::endl;
}

// Case 8: Practical CGI script execution pattern
void case_8_realistic_cgi()
{
    print_separator("Case 8: Realistic CGI Pattern");
    
    // This simulates what happens in actual webserv
    
    // HTTP Request arrives
    std::string request_method = "GET";
    std::string request_path = "/cgi-bin/greet.py";
    std::string query_string = "name=Alice&greeting=hello";
    std::string http_host = "example.com:8080";
    std::string remote_addr = "203.0.113.50";
    
    std::cout << "Parent: Received HTTP REQUEST from client" << std::endl;
    std::cout << "  Method: " << request_method << std::endl;
    std::cout << "  Path: " << request_path << std::endl;
    std::cout << "  Query: " << query_string << std::endl;
    std::cout << "  Host: " << http_host << std::endl;
    std::cout << "  Client IP: " << remote_addr << std::endl;
    
    // SERVER EXTRACTS AND SETS VARIABLES
    setenv("REQUEST_METHOD", request_method.c_str(), 1);
    setenv("SCRIPT_NAME", request_path.c_str(), 1);
    setenv("QUERY_STRING", query_string.c_str(), 1);
    setenv("HTTP_HOST", http_host.c_str(), 1);
    setenv("REMOTE_ADDR", remote_addr.c_str(), 1);
    setenv("SERVER_NAME", "example.com", 1);
    setenv("SERVER_PORT", "8080", 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    
    std::cout << "\nParent: Environment ready. Would fork and exec here." << std::endl;
    std::cout << "  ./greet.py (which reads environ): " << std::endl;
    
    // Now imagine greet.py does:
    // #!/usr/bin/env python
    // import os
    // name = os.environ.get('QUERY_STRING', '').split('=')[1]
    // print("Hello, " + name + "!")
    
    std::cout << "  (Would output: Hello, Alice!)" << std::endl;
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  Environment Variables for CGI                    █" << std::endl;
    std::cout << "█  Educational Test Cases                           █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    case_1_read_environment();
    case_2_setenv_basic();
    case_3_inheritance();
    case_4_cgi_setup();
    case_5_post_environment();
    case_6_execute_with_env();
    case_7_overwrite_behavior();
    case_8_realistic_cgi();
    
    print_separator("All cases completed!");
    std::cout << "\nKey takeaway: Set ALL variables BEFORE fork(), they are inherited." << std::endl;
    
    return 0;
}
