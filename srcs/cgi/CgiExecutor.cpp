#include <CgiExecutor.hpp>
#include <CgiPipeManager.hpp>
#include <CgiException.hpp>
#include <StringUtils.hpp>
#include <Logger.hpp>

#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <CgiEnvBuilder.hpp>

CgiResult CgiExecutor::execute(const HttpRequest& request, LocationConfig& location_config)
{
    CgiPipeManager pipe;
    const std::string& request_path = request.path.empty() ? request.uri : request.path;
    std::size_t dotPos = request_path.rfind('.');

    if (dotPos == std::string::npos) {
        throw CgiException("No CGI handler configured for request path without extension: " + request_path);
    }

    std::string file_extension = request_path.substr(dotPos);
    std::string cgi_interpreter = location_config.cgi_handlers[file_extension];
    std::string cgi_script_path = location_config.root + request_path;

    if (cgi_interpreter.empty()) {
        std::string error_msg = "No CGI handler configured for extension: " + file_extension;
        return CgiResult(CgiResult::NO_HANDLER, error_msg);
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::string error_msg = "Failed to fork process: " + std::string(strerror(errno));
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }
    if (pid == 0) {
        pipe.setup_child_process();
        const char* args[] = { cgi_interpreter.c_str(), cgi_script_path.c_str(), NULL };
        CgiEnvBuilder env_builder(request);
        execve(args[0], (char* const*)args, env_builder.getEnvp());

        exit(127);
    }
    pipe.setup_parent_process();
    std::string data = request.method == "POST" ? request.body : "";
    pipe.write_to_child(data);

    std::string output = "";
    try {
        output = pipe.read_from_child(pid);
    } catch(const CgiException& e) {
        std::string error_msg = "CGI execution error: " + std::string(e.what());
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }

    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        std::string error_msg = "CGI script exited with code: "
            + StringUtils::to_string(WEXITSTATUS(status));
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }

    return CgiResult(CgiResult::SUCCESS, output);
}
