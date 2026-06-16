/* ************************************************************************** */
/*  test_session_persistence.cpp — Session and Cookie Flow Tests              */
/* ************************************************************************** */

#include "minitest.hpp"
#include <CgiHandler.hpp>
#include <CgiExecutor.hpp>
#include <HttpRequest.hpp>
#include <LocationConfig.hpp>
#include <ServerConfig.hpp>
#include <StringUtils.hpp>
#include <unistd.h>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <errno.h>

struct CgiResult {
    CgiExecutionStatus status;
    std::string output;
};

namespace {
    bool pythonAvailable() {
        return system("which python3 > /dev/null 2>&1") == 0;
    }

    CgiResult sync_execute(CgiExecutor& executor, const HttpRequest& req, const LocationConfig& config, const ServerConfig& srv) {
        CgiProcessInfo info = executor.start_cgi(req, config, srv, -1);
        std::string output;
        char buffer[4096];
        size_t body_written = 0;
        bool stdin_closed = false;
        bool stdout_done = false;
        while (!stdout_done) {
            if (!stdin_closed) {
                if (body_written < req.body.length()) {
                    ssize_t wn = write(info.stdin_fd, req.body.c_str() + body_written, req.body.length() - body_written);
                    if (wn > 0) body_written += wn;
                    else if (wn == -1 && errno != EAGAIN && errno != EWOULDBLOCK) { close(info.stdin_fd); stdin_closed = true; }
                } else { close(info.stdin_fd); stdin_closed = true; }
            }
            ssize_t rn = read(info.stdout_fd, buffer, sizeof(buffer));
            if (rn > 0) output.append(buffer, rn);
            else if (rn == 0) stdout_done = true;
            else if (rn == -1 && errno != EAGAIN && errno != EWOULDBLOCK) stdout_done = true;
            if (!stdout_done) usleep(100);
        }
        int status = 0;
        waitpid(info.pid, &status, 0);
        close(info.stdout_fd);
        if (!stdin_closed) close(info.stdin_fd);
        CgiExecutionStatus res_status = CGI_EXEC_SUCCESS;
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) res_status = CGI_EXEC_ERROR;
        { CgiResult r; r.status = res_status; r.output = output; return r; }
    }
}

TEST(SessionPersistence, CookieFlow) {
    if (!pythonAvailable()) {
        return;
    }

    // 1. Setup: Clean session directory
    system("rm -rf /tmp/webserv_sessions");
    
    // The script is at www/cgi/session.py relative to project root
    // But the CgiExecutor expects the script relative to config.root
    
    HttpRequest req;
    req.setMethod("GET")
       .setUri("/www/cgi/session.py").setPath("/www/cgi/session.py")
       .setHttpVersion("1.1");

    LocationConfig config;
    // We assume we are running from the tests/ directory, so project root is ../
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string s_cwd(cwd);
        size_t last_slash = s_cwd.find_last_of('/');
        config.root = s_cwd.substr(0, last_slash); // Parent dir
    } else {
        config.root = "..";
    }
    
    config.cgi_handlers[".py"] = "/usr/bin/python3";

		ServerConfig srv;
    CgiExecutor executor;

    // --- FIRST VISIT (No Cookie) ---
    CgiResult result1 = sync_execute(executor, req, config, srv);
    
    // Validate Visit Count 1
    ASSERT_TRUE(result1.output.find("Visit Count: 1") != std::string::npos);
    // Validate Set-Cookie presence
    size_t cookiePos = result1.output.find("Set-Cookie: session_id=");
    ASSERT_TRUE(cookiePos != std::string::npos);
    
    // Extract Session ID
    std::string startMarker = "session_id=";
    size_t start = result1.output.find(startMarker, cookiePos) + startMarker.length();
    size_t end = result1.output.find(";", start);
    
    // Validate other cookie attributes
    ASSERT_TRUE(result1.output.find("Path=/") != std::string::npos);
    ASSERT_TRUE(result1.output.find("HttpOnly") != std::string::npos);
    
    if (end == std::string::npos) end = result1.output.find("\r", start);
    if (end == std::string::npos) end = result1.output.find("\n", start);
    
    std::string sessionId = result1.output.substr(start, end - start);
    ASSERT_TRUE(!sessionId.empty());

    // --- SECOND VISIT (With received cookie) ---
    HttpRequest req2;
    req2.setMethod("GET")
        .setUri("/www/cgi/session.py").setPath("/www/cgi/session.py")
        .setHttpVersion("1.1")
        .addHeader("Cookie", "session_id=" + sessionId);

    CgiResult result2 = sync_execute(executor, req2, config, srv);
    
    // Validate Visit Count 2
    ASSERT_TRUE(result2.output.find("Visit Count: 2") != std::string::npos);
    // Session ID should remain the same
    ASSERT_TRUE(result2.output.find("Session ID: " + sessionId) != std::string::npos);

    // --- THIRD VISIT (Invalid/Unknown Cookie) ---
    HttpRequest req3;
    req3.setMethod("GET")
        .setUri("/www/cgi/session.py").setPath("/www/cgi/session.py")
        .setHttpVersion("1.1")
        .addHeader("Cookie", "session_id=nonexistent_session_999");

    CgiResult result3 = sync_execute(executor, req3, config, srv);
    
    // Should reset count to 1 and generate a new ID
    ASSERT_TRUE(result3.output.find("Visit Count: 1") != std::string::npos);
    ASSERT_TRUE(result3.output.find("Session ID: " + sessionId) == std::string::npos);
    
    // Cleanup
    system("rm -rf /tmp/webserv_sessions");
}

TEST(SessionPersistence, MultipleCookies) {
    if (!pythonAvailable()) return;

    system("rm -rf /tmp/webserv_sessions");
    LocationConfig config;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string s_cwd(cwd);
        size_t last_slash = s_cwd.find_last_of('/');
        config.root = s_cwd.substr(0, last_slash);
    } else { config.root = ".."; }
    config.cgi_handlers[".py"] = "/usr/bin/python3";
		ServerConfig srv;
    CgiExecutor executor;

    // First visit to get an ID
    HttpRequest req1;
    req1.setMethod("GET").setUri("/www/cgi/session.py").setPath("/www/cgi/session.py").setHttpVersion("1.1");
    CgiResult res1 = sync_execute(executor, req1, config, srv);
    
    std::string sidMarker = "session_id=";
    size_t start = res1.output.find(sidMarker) + sidMarker.length();
    size_t end = res1.output.find(";", start);
    std::string sessionId = res1.output.substr(start, end - start);

    // Second visit with multiple cookies
    HttpRequest req2;
    req2.setMethod("GET")
        .setUri("/www/cgi/session.py").setPath("/www/cgi/session.py")
        .setHttpVersion("1.1")
        .addHeader("Cookie", "theme=dark; session_id=" + sessionId + "; lang=en-US");

    CgiResult res2 = sync_execute(executor, req2, config, srv);
    
    ASSERT_TRUE(res2.output.find("Visit Count: 2") != std::string::npos);
    ASSERT_TRUE(res2.output.find("Session ID: " + sessionId) != std::string::npos);
}

TEST(SessionPersistence, CorruptedSessionFile) {
    if (!pythonAvailable()) return;

    system("mkdir -p /tmp/webserv_sessions");
    std::string corruptedId = "corrupt_123";
    std::string filePath = "/tmp/webserv_sessions/session_" + corruptedId;
    
    // Write invalid data (not an integer)
    std::ofstream f(filePath.c_str());
    f << "INVALID_DATA";
    f.close();

    LocationConfig config;
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::string s_cwd(cwd);
        size_t last_slash = s_cwd.find_last_of('/');
        config.root = s_cwd.substr(0, last_slash);
    } else { config.root = ".."; }
    config.cgi_handlers[".py"] = "/usr/bin/python3";
		ServerConfig srv;
    CgiExecutor executor;

    HttpRequest req;
    req.setMethod("GET")
       .setUri("/www/cgi/session.py").setPath("/www/cgi/session.py")
       .setHttpVersion("1.1")
       .addHeader("Cookie", "session_id=" + corruptedId);

    CgiResult res = sync_execute(executor, req, config, srv);
    
    // The script should handle the ValueError and reset visits to 0 + 1 = 1
    ASSERT_TRUE(res.output.find("Visit Count: 1") != std::string::npos);
    
    system("rm -rf /tmp/webserv_sessions");
}

MINITEST_MAIN()
