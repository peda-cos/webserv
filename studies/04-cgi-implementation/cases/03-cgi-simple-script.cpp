// 03-cgi-simple-script.cpp
// Educational case: Simple CGI script executor
//
// Compile: g++ -std=c++98 -Wall -Wextra 03-cgi-simple-script.cpp -o 03-cgi-simple-script
// Run: ./03-cgi-simple-script
//
// This demonstrates:
// 1. Creating a simple C++ CGI script (alternative to Python/PHP)
// 2. Reading environment variables in the script
// 3. Handling GET query parameters
// 4. Generating HTTP-like output
// 5. Executor pattern that runs external scripts

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

// ============================================================================
// SECTION 1: A SIMPLE C++ CGI SCRIPT (Normally would be separate executable)
// ============================================================================

// This would normally be in a separate file like "/tmp/simple_script.cpp"
// But for this educational example, we'll compile it together

const char* SIMPLE_CGI_SCRIPT = 
"#!/usr/bin/env python\n"
"import os\n"
"import sys\n"
"\n"
"# Read environment\n"
"method = os.environ.get('REQUEST_METHOD', 'GET')\n"
"query_string = os.environ.get('QUERY_STRING', '')\n"
"content_length = os.environ.get('CONTENT_LENGTH', '0')\n"
"\n"
"# Parse query string (simple parser)\n"
"def parse_query(qs):\n"
"    params = {}\n"
"    if qs:\n"
"        pairs = qs.split('&')\n"
"        for pair in pairs:\n"
"            if '=' in pair:\n"
"                k, v = pair.split('=', 1)\n"
"                params[k] = v\n"
"    return params\n"
"\n"
"# GET request\n"
"if method == 'GET':\n"
"    params = parse_query(query_string)\n"
"    name = params.get('name', 'World')\n"
"    \n"
"    print(\"Content-Type: text/html\")\n"
"    print(\"\")\n"
"    print(\"<html><body>\")\n"
"    print(\"<h1>Hello, \" + name + \"!</h1>\")\n"
"    print(\"<p>You were processed by CGI.</p>\")\n"
"    print(\"</body></html>\")\n"
"    sys.exit(0)\n"
"\n"
"# POST request\n"
"elif method == 'POST':\n"
"    body = sys.stdin.read()\n"
"    params = parse_query(body)\n"
"    username = params.get('username', 'Guest')\n"
"    \n"
"    print(\"Content-Type: text/html\")\n"
"    print(\"\")\n"
"    print(\"<html><body>\")\n"
"    print(\"<h1>Login successful!</h1>\")\n"
"    print(\"<p>Welcome, \" + username + \"</p>\")\n"
"    print(\"</body></html>\")\n"
"    sys.exit(0)\n"
"\n"
"# Unknown method\n"
"else:\n"
"    print(\"HTTP/1.1 405 Method Not Allowed\")\n"
"    sys.exit(1)\n";

void print_separator(const std::string& title)
{
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << "═══════════════════════════════════════════════════════" << std::endl;
}

// Helper: Convert map to query string
std::string params_to_query_string(const std::map<std::string, std::string>& params)
{
    std::string qs = "";
    for (std::map<std::string, std::string>::const_iterator it = params.begin();
         it != params.end(); ++it) {
        if (!qs.empty()) qs += "&";
        qs += it->first + "=" + it->second;
    }
    return qs;
}

// Helper: Size to string
std::string size_to_string(size_t value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// ============================================================================
// SECTION 2: CGI EXECUTOR (What the webserver does)
// ============================================================================

// Structure to represent an HTTP request
struct HttpRequest {
    std::string method;
    std::string script;
    std::string query_string;
    std::string body;
    
    HttpRequest(const std::string& m, const std::string& s, 
                const std::string& q, const std::string& b)
        : method(m), script(s), query_string(q), body(b) {}
};

// Execute CGI script and return response
std::string execute_cgi(const HttpRequest& request)
{
    // For this example, we'll execute a Python script
    // In real webserv, could be any executable
    
    // Create the script file temporarily
    const char* script_path = "/tmp/test_cgi_script.py";
    
    // Write script to file
    std::ofstream script_file(script_path);
    script_file << SIMPLE_CGI_SCRIPT;
    script_file.close();
    
    // Make it executable
    chmod(script_path, 0755);
    
    // Create pipes for communication
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        std::cerr << "Pipe failed: " << strerror(errno) << std::endl;
        return "ERROR: Could not create pipes";
    }
    
    // Fork
    pid_t pid = fork();
    
    if (pid == -1) {
        std::cerr << "Fork failed: " << strerror(errno) << std::endl;
        return "ERROR: Could not fork";
    }
    
    if (pid == 0) {
        // ===== CHILD PROCESS =====
        
        // Setup pipes
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        // Close pipe file descriptors
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        
        // Set CGI environment variables
        setenv("REQUEST_METHOD", request.method.c_str(), 1);
        setenv("SCRIPT_NAME", request.script.c_str(), 1);
        
        if (request.method == "GET") {
            setenv("QUERY_STRING", request.query_string.c_str(), 1);
            setenv("CONTENT_LENGTH", "0", 1);
        } else if (request.method == "POST") {
            setenv("QUERY_STRING", "", 1);
            std::string len_str = size_to_string(request.body.size());
            setenv("CONTENT_LENGTH", len_str.c_str(), 1);
            setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        }
        
        setenv("SERVER_NAME", "localhost", 1);
        setenv("SERVER_PORT", "8080", 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        
        // Execute Python interpreter with script
        const char* args[] = { "python", script_path, NULL };
        execvp("python3", (char* const*)args);
        
        // If exec fails
        std::cerr << "Exec failed: " << strerror(errno) << std::endl;
        exit(127);
    }
    else {
        // ===== PARENT PROCESS =====
        
        // Close unused pipe ends
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Write request body to child (if POST)
        if (request.method == "POST" && !request.body.empty()) {
            write(stdin_pipe[1], request.body.c_str(), request.body.size());
        }
        
        // Close write end to signal EOF
        close(stdin_pipe[1]);
        
        // Read response from child
        char buffer[4096];
        std::string response = "";
        
        while (true) {
            ssize_t bytes = read(stdout_pipe[0], buffer, sizeof(buffer));
            if (bytes <= 0) break;
            response.append(buffer, bytes);
        }
        
        close(stdout_pipe[0]);
        
        // Wait for child to finish
        int status;
        waitpid(pid, &status, 0);
        
        // Check exit status
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                std::string error = "Script exited with code ";
                error += size_to_string(exit_code);
                return error;
            }
        }
        
        // Clean up
        unlink(script_path);
        
        return response;
    }
    
    return "";  // Never reached
}

// ============================================================================
// SECTION 3: TEST CASES
// ============================================================================

void case_1_simple_get()
{
    print_separator("Case 1: Simple GET Request");
    
    std::map<std::string, std::string> params;
    params["name"] = "Alice";
    
    HttpRequest request("GET", "/cgi-bin/script.py", 
                       params_to_query_string(params), "");
    
    std::cout << "Client: Requesting GET /cgi-bin/script.py?name=Alice\n" << std::endl;
    
    std::string response = execute_cgi(request);
    
    std::cout << "Server received from CGI:\n" << std::endl;
    std::cout << response << std::endl;
}

void case_2_simple_post()
{
    print_separator("Case 2: Simple POST Request");
    
    HttpRequest request("POST", "/cgi-bin/script.py", "", 
                       "username=admin");
    
    std::cout << "Client: Requesting POST /cgi-bin/script.py\n"
              << "Body: username=admin\n" << std::endl;
    
    std::string response = execute_cgi(request);
    
    std::cout << "Server received from CGI:\n" << std::endl;
    std::cout << response << std::endl;
}

void case_3_multiple_params()
{
    print_separator("Case 3: Multiple Query Parameters");
    
    std::map<std::string, std::string> params;
    params["name"] = "Bob";
    params["greeting"] = "Hi";
    
    HttpRequest request("GET", "/cgi-bin/script.py", 
                       params_to_query_string(params), "");
    
    std::cout << "Client: Requesting GET with multiple params\n" << std::endl;
    
    std::string response = execute_cgi(request);
    
    std::cout << "Server received from CGI:\n" << std::endl;
    std::cout << response << std::endl;
}

void case_4_post_with_data()
{
    print_separator("Case 4: POST with Multiple Form Fields");
    
    std::string body = "username=admin&password=secret&email=admin@example.com";
    
    HttpRequest request("POST", "/cgi-bin/login.py", "", body);
    
    std::cout << "Client: Requesting POST\n"
              << "Body: " << body << "\n" << std::endl;
    
    std::string response = execute_cgi(request);
    
    std::cout << "Server received from CGI:\n" << std::endl;
    std::cout << response << std::endl;
}

void case_5_empty_query()
{
    print_separator("Case 5: GET with No Query Parameters");
    
    HttpRequest request("GET", "/cgi-bin/script.py", "", "");
    
    std::cout << "Client: Requesting GET /cgi-bin/script.py (no params)\n" << std::endl;
    
    std::string response = execute_cgi(request);
    
    std::cout << "Server received from CGI:\n" << std::endl;
    std::cout << response << std::endl;
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  Simple CGI Script Executor                       █" << std::endl;
    std::cout << "█  Educational Test Cases                           █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    std::cout << "\nNote: These examples require Python to be installed." << std::endl;
    std::cout << "If Python is not available, modify to use shell scripts instead.\n" << std::endl;
    
    try {
        case_1_simple_get();
        case_2_simple_post();
        case_3_multiple_params();
        case_4_post_with_data();
        case_5_empty_query();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    print_separator("All cases completed!");
    
    return 0;
}
