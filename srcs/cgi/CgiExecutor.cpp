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
#include <CgiEnvBuilder.hpp>
#include <CgiUtils.hpp>

namespace {
    static std::string script_directory(const std::string& script_path) {
        std::string::size_type slash_pos = script_path.find_last_of('/');
        if (slash_pos == std::string::npos) {
            return ".";
        }
        if (slash_pos == 0) {
            return "/";
        }
        return script_path.substr(0, slash_pos);
    }
}

CgiResult CgiExecutor::execute(const HttpRequest& request, const LocationConfig& location_config) const
{
    CgiPipeManager pipe;
    const std::string& request_path = request.path.empty() ? request.uri : request.path;
    std::string file_extension;
    std::size_t slash_pos = 0;

    if (!CgiUtils::extract_extension(request_path, file_extension, slash_pos)) {
        throw CgiException("Request path does not contain an extension: " + request_path);
    }

    std::map<std::string, std::string>::const_iterator handler_it = location_config.cgi_handlers.find(file_extension);
    if (handler_it == location_config.cgi_handlers.end()) {
        std::string error_msg = "No CGI handler configured for extension: " + file_extension;
        Logger::warn(error_msg);
        return CgiResult(CgiResult::NO_HANDLER, error_msg);
    }

    const std::string& cgi_interpreter = handler_it->second;
    std::string request_script_path = request_path.substr(0, slash_pos);
    std::string cgi_script_path = CgiUtils::absolute_path(CgiUtils::resolve_script_path(request_script_path, location_config));

    Logger::info("Launching CGI script: " + request_script_path + " using " + cgi_interpreter);

    pid_t pid = fork();
    if (pid == -1) {
        std::string error_msg = "Failed to fork process: " + std::string(strerror(errno));
        Logger::error(error_msg);
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }
    if (pid == 0) {
        pipe.setup_child_process();

        std::string cgi_work_dir = script_directory(cgi_script_path);
        if (chdir(cgi_work_dir.c_str()) != 0) {
            exit(126);
        }

        const char* args[3];
        args[0] = cgi_interpreter.c_str();
        args[1] = cgi_script_path.c_str();
        args[2] = NULL;
        CgiEnvBuilder env_builder(request, location_config);
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
        std::string exception_message = std::string(e.what());
        if (exception_message.find("timed out") != std::string::npos) {
            Logger::warn("CGI timed out for script: " + request_script_path);
            return CgiResult(CgiResult::TIMEOUT, "CGI execution timed out");
        }
        std::string error_msg = "CGI execution error: " + exception_message;
        Logger::error(error_msg);
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }

    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        std::string error_msg = "CGI script exited with code: "
            + StringUtils::to_string(WEXITSTATUS(status));
        Logger::warn(error_msg + " (script: " + request_script_path + ")");
        return CgiResult(CgiResult::EXECUTION_ERROR, error_msg);
    }

    Logger::info("CGI script executed successfully: " + request_script_path);

    return CgiResult(CgiResult::SUCCESS, output);
}
