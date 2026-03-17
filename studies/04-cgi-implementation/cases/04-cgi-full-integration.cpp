// 04-cgi-full-integration.cpp
// Educational case: Full CGI integration pattern
//
// Compile: g++ -std=c++98 -Wall -Wextra 04-cgi-full-integration.cpp -o 04-cgi-full-integration
// Run: ./04-cgi-full-integration
//
// This demonstrates:
// 1. How CGI fits into the webserver architecture
// 2. Detection of CGI vs static file requests
// 3. Complete request-response cycle with CGI
// 4. Error handling and response generation
// 5. Integration with config-based routing

#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <sstream>

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
// DATA STRUCTURES
// ============================================================================

// Represents a URL location configuration
struct LocationConfig {
    std::string path;           // e.g., "/cgi-bin/"
    bool is_cgi;                // Is this a CGI location?
    std::string root;           // Document root
    
    LocationConfig(const std::string& p, bool cgi, const std::string& r)
        : path(p), is_cgi(cgi), root(r) {}
};

// Server configuration (simplified)
struct ServerConfig {
    std::string name;
    std::vector<LocationConfig> locations;
    
    ServerConfig(const std::string& n) : name(n) {}
};

// HTTP Request structure
struct HttpRequest {
    std::string method;         // GET, POST, DELETE
    std::string path;           // /cgi-bin/script.py or /index.html
    std::string query_string;   // a=1&b=2
    std::string body;           // POST/PUT body
    std::map<std::string, std::string> headers;
    
    HttpRequest(const std::string& m, const std::string& p)
        : method(m), path(p), query_string(""), body("") {}
};

// HTTP Response structure
struct HttpResponse {
    int status_code;
    std::string content_type;
    std::string body;
    
    HttpResponse(int code = 200, const std::string& type = "text/html")
        : status_code(code), content_type(type), body("") {}
    
    std::string to_string() {
        std::string response = "";
        
        // Status line
        response += "HTTP/1.1 ";
        response += size_to_string(status_code);
        
        switch (status_code) {
            case 200: response += " OK"; break;
            case 404: response += " Not Found"; break;
            case 500: response += " Internal Server Error"; break;
            case 403: response += " Forbidden"; break;
            default: response += " Unknown"; break;
        }
        response += "\r\n";
        
        // Headers
        response += "Content-Type: " + content_type + "\r\n";
        response += "Content-Length: " + size_to_string(body.size()) + "\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        
        // Body
        response += body;
        
        return response;
    }
};

// ============================================================================
// LOCATION MATCHING
// ============================================================================

// Match request path to location config
const LocationConfig* match_location(const std::string& path, 
                                     const ServerConfig& server)
{
    // Simple prefix matching
    // In real nginx, this is more complex (regex, priority order)
    
    for (size_t i = 0; i < server.locations.size(); ++i) {
        const LocationConfig& loc = server.locations[i];
        
        if (path.find(loc.path) == 0) {
            return &loc;
        }
    }
    
    return NULL;
}

// ============================================================================
// CGI EXECUTION
// ============================================================================

HttpResponse execute_cgi_script(const std::string& script_path,
                               const HttpRequest& request,
                               const std::string& server_name)
{
    // Check if script exists and is executable
    struct stat st;
    if (stat(script_path.c_str(), &st) == -1) {
        HttpResponse resp(404, "text/html");
        resp.body = "<html><body><h1>404 Not Found</h1></body></html>";
        return resp;
    }
    
    if (!(st.st_mode & S_IXUSR)) {
        HttpResponse resp(403, "text/html");
        resp.body = "<html><body><h1>403 Forbidden</h1></body></html>";
        return resp;
    }
    
    // Create pipes
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        HttpResponse resp(500, "text/html");
        resp.body = "<html><body><h1>500 Internal Server Error</h1>"
                    "<p>Could not create pipes</p></body></html>";
        return resp;
    }
    
    // Fork
    pid_t pid = fork();
    
    if (pid == -1) {
        HttpResponse resp(500, "text/html");
        resp.body = "<html><body><h1>500 Internal Server Error</h1>"
                    "<p>Could not fork</p></body></html>";
        return resp;
    }
    
    if (pid == 0) {
        // ===== CHILD PROCESS =====
        
        // Redirect pipes
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        // Close file descriptors
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        
        // Set CGI environment variables
        setenv("REQUEST_METHOD", request.method.c_str(), 1);
        setenv("SCRIPT_NAME", request.path.c_str(), 1);
        setenv("QUERY_STRING", request.query_string.c_str(), 1);
        setenv("SERVER_NAME", server_name.c_str(), 1);
        setenv("SERVER_PORT", "8080", 1);
        setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
        
        if (request.method == "POST") {
            std::string len = size_to_string(request.body.size());
            setenv("CONTENT_LENGTH", len.c_str(), 1);
            setenv("CONTENT_TYPE", "application/x-www-form-urlencoded", 1);
        } else {
            setenv("CONTENT_LENGTH", "0", 1);
        }
        
        // Execute script
        const char* args[] = { script_path.c_str(), NULL };
        execv(script_path.c_str(), (char* const*)args);
        
        // If we get here, exec failed
        perror("execv");
        exit(127);
    }
    else {
        // ===== PARENT PROCESS =====
        
        // Close unused ends
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        // Write POST body if needed
        if (request.method == "POST" && !request.body.empty()) {
            write(stdin_pipe[1], request.body.c_str(), request.body.size());
        }
        
        // Close write end to signal EOF
        close(stdin_pipe[1]);
        
        // Read response from script
        char buffer[4096];
        std::string cgi_output = "";
        
        while (true) {
            ssize_t bytes = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
            if (bytes <= 0) break;
            cgi_output.append(buffer, bytes);
        }
        
        close(stdout_pipe[0]);
        
        // Wait for child
        int status;
        waitpid(pid, &status, 0);
        
        // Check exit code
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                HttpResponse resp(500, "text/html");
                resp.body = "<html><body><h1>500 Internal Server Error</h1>"
                            "<p>Script exit code: " + size_to_string(exit_code) + 
                            "</p></body></html>";
                return resp;
            }
        }
        
        // Return CGI output as response
        HttpResponse resp(200, "text/html");
        resp.body = cgi_output;
        return resp;
    }
    
    return HttpResponse(500);  // Never reached
}

// ============================================================================
// REQUEST HANDLER (Decision Logic)
// ============================================================================

HttpResponse handle_request(const HttpRequest& request, 
                           const ServerConfig& server)
{
    std::cout << "Server: Processing " << request.method << " " 
              << request.path;
    if (!request.query_string.empty()) {
        std::cout << "?" << request.query_string;
    }
    std::cout << std::endl;
    
    // Match location
    const LocationConfig* location = match_location(request.path, server);
    
    if (!location) {
        std::cout << "Server: No matching location" << std::endl;
        HttpResponse resp(404, "text/html");
        resp.body = "<html><body><h1>404 Not Found</h1>"
                    "<p>Path not found in configuration</p></body></html>";
        return resp;
    }
    
    std::cout << "Server: Matched location: " << location->path << std::endl;
    
    // Check: Is this CGI?
    if (location->is_cgi) {
        std::cout << "Server: This is a CGI location" << std::endl;
        
        // For this example, construct script path
        // In real server, would parse the path properly
        std::string script_path = location->root + request.path;
        
        std::cout << "Server: Script path: " << script_path << std::endl;
        std::cout << "Server: Forking and executing CGI script..." << std::endl;
        
        return execute_cgi_script(script_path, request, server.name);
    }
    else {
        std::cout << "Server: This is a STATIC file location" << std::endl;
        std::cout << "Server: Would serve static file here (not implemented)" << std::endl;
        
        HttpResponse resp(200, "text/html");
        resp.body = "<html><body><h1>Static File</h1>"
                    "<p>File serving not implemented in this educational example</p>"
                    "</body></html>";
        return resp;
    }
}

// ============================================================================
// TEST CASES
// ============================================================================

void case_1_cgi_detection()
{
    print_separator("Case 1: CGI Location Detection");
    
    // Setup server config with multiple locations
    ServerConfig server("example.com");
    server.locations.push_back(LocationConfig("/", false, "/var/www/html"));
    server.locations.push_back(LocationConfig("/cgi-bin/", true, "/var/www"));
    server.locations.push_back(LocationConfig("/api/", true, "/var/www"));
    
    std::cout << "Server Configuration:" << std::endl;
    std::cout << "  / → Static (root: /var/www/html)" << std::endl;
    std::cout << "  /cgi-bin/ → CGI (root: /var/www)" << std::endl;
    std::cout << "  /api/ → CGI (root: /var/www)" << std::endl;
    
    std::cout << "\nTest: Which are CGI?" << std::endl;
    
    struct TestCase {
        std::string path;
        bool expected_cgi;
    };
    
    TestCase tests[] = {
        { "/index.html", false },
        { "/cgi-bin/script.py", true },
        { "/api/users.py", true },
        { "/images/photo.jpg", false },
    };
    
    for (int i = 0; i < 4; ++i) {
        const LocationConfig* loc = match_location(tests[i].path, server);
        bool is_cgi = loc ? loc->is_cgi : false;
        
        std::cout << "  " << tests[i].path << " → ";
        std::cout << (is_cgi ? "CGI" : "Static");
        
        if (is_cgi == tests[i].expected_cgi) {
            std::cout << " ✓" << std::endl;
        } else {
            std::cout << " ✗ (MISMATCH)" << std::endl;
        }
    }
}

void case_2_request_routing()
{
    print_separator("Case 2: Request Routing Decision");
    
    ServerConfig server("localhost");
    server.locations.push_back(LocationConfig("/", false, "/var/www"));
    server.locations.push_back(LocationConfig("/cgi-bin/", true, "/var/www"));
    
    struct RequestTest {
        std::string method;
        std::string path;
        std::string query;
    };
    
    RequestTest requests[] = {
        { "GET", "/index.html", "" },
        { "GET", "/cgi-bin/echo.py", "msg=hello" },
        { "POST", "/cgi-bin/login.php", "" },
        { "GET", "/assets/style.css", "" },
    };
    
    for (int i = 0; i < 4; ++i) {
        HttpRequest req(requests[i].method, requests[i].path);
        req.query_string = requests[i].query;
        
        std::cout << "Request: " << requests[i].method << " " 
                  << requests[i].path;
        if (!requests[i].query.empty()) {
            std::cout << "?" << requests[i].query;
        }
        std::cout << std::endl;
        
        const LocationConfig* loc = match_location(req.path, server);
        if (loc && loc->is_cgi) {
            std::cout << "  → Route to CGI executor" << std::endl;
        } else if (loc) {
            std::cout << "  → Serve static file" << std::endl;
        } else {
            std::cout << "  → 404 Not Found" << std::endl;
        }
        std::cout << std::endl;
    }
}

void case_3_http_response_format()
{
    print_separator("Case 3: HTTP Response Format");
    
    std::cout << "Demonstrating HTTP Response format:" << std::endl;
    
    // 200 OK response
    HttpResponse success(200, "text/html");
    success.body = "<html><body>Success!</body></html>";
    
    std::cout << "\n--- 200 OK Response ---" << std::endl;
    std::cout << success.to_string() << std::endl;
    
    // 404 Not Found response
    HttpResponse notfound(404, "text/html");
    notfound.body = "<html><body>Not Found</body></html>";
    
    std::cout << "\n--- 404 Not Found Response ---" << std::endl;
    std::cout << notfound.to_string() << std::endl;
    
    // 500 Internal Server Error response
    HttpResponse error(500, "text/html");
    error.body = "<html><body>Internal Server Error</body></html>";
    
    std::cout << "\n--- 500 Internal Server Error Response ---" << std::endl;
    std::cout << error.to_string() << std::endl;
}

void case_4_config_based_routing()
{
    print_separator("Case 4: Configuration-Based Routing");
    
    std::cout << "Server configuration in .conf format:" << std::endl;
    std::cout << "\n"
    "    server {\n"
    "        listen 8080;\n"
    "        server_name example.com;\n"
    "        root /var/www/html;\n"
    "        \n"
    "        location / {\n"
    "            # Static files\n"
    "            index index.html;\n"
    "        }\n"
    "        \n"
    "        location /cgi-bin/ {\n"
    "            # CGI scripts (all files are executables)\n"
    "        }\n"
    "        \n"
    "        location /api/ {\n"
    "            # API endpoints (also CGI)\n"
    "        }\n"
    "    }\n"
    "    " << std::endl;
    
    std::cout << "When parsing this .conf:" << std::endl;
    std::cout << "  1. Each 'location' block → LocationConfig struct" << std::endl;
    std::cout << "  2. No special directive → is_cgi = false (static)" << std::endl;
    std::cout << "  3. Future directive like 'cgi on;' → is_cgi = true" << std::endl;
    std::cout << std::endl;
    std::cout << "Routing logic:" << std::endl;
    std::cout << "  if (location.is_cgi) {" << std::endl;
    std::cout << "    execute_cgi_script(path, request);" << std::endl;
    std::cout << "  } else {" << std::endl;
    std::cout << "    serve_static_file(path);" << std::endl;
    std::cout << "  }" << std::endl;
}

int main()
{
    std::cout << "\n" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "█  Full CGI Integration Architecture                █" << std::endl;
    std::cout << "█  Educational Test Cases                           █" << std::endl;
    std::cout << "█                                                    █" << std::endl;
    std::cout << "████████████████████████████████████████████████████" << std::endl;
    
    case_1_cgi_detection();
    case_2_request_routing();
    case_3_http_response_format();
    case_4_config_based_routing();
    
    print_separator("All cases completed!");
    std::cout << "\nKey Takeaway:" << std::endl;
    std::cout << "CGI is just one handler among many. Detection (static vs CGI)" << std::endl;
    std::cout << "happens based on location configuration, not file extension." << std::endl;
    
    return 0;
}
