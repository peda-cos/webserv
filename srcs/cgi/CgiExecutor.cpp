#include <CgiExecutor.hpp>
#include <CgiPipeManager.hpp>
#include <CgiException.hpp>
#include <Logger.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <CgiEnvBuilder.hpp>
#include <CgiUtils.hpp>
#include <cstdlib>

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

CgiProcessInfo CgiExecutor::start_cgi(const HttpRequest& request, const LocationConfig& location_config) const
{
    CgiPipeManager pipe_manager;
    const std::string& request_path = request.path.empty() ? request.uri : request.path;
    std::string file_extension;
    std::size_t slash_pos = 0;

    if (!CgiUtils::extract_extension(request_path, file_extension, slash_pos)) {
        throw CgiException("Request path does not contain an extension: " + request_path);
    }

    std::map<std::string, std::string>::const_iterator handler_it = location_config.cgi_handlers.find(file_extension);
    const std::string& cgi_interpreter = handler_it->second;
    std::string request_script_path = request_path.substr(0, slash_pos);
    std::string cgi_script_path = CgiUtils::absolute_path(CgiUtils::resolve_script_path(request_script_path, location_config));

    pid_t pid = fork();
    if (pid == -1) {
        throw CgiException("Failed to fork process: " + std::string(strerror(errno)));
    }
    if (pid == 0) {
        pipe_manager.setup_child_process();
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

    int stdin_fd = pipe_manager.get_stdin_pipe()[1];
    int stdout_fd = pipe_manager.get_stdout_pipe()[0];

    fcntl(stdin_fd, F_SETFL, O_NONBLOCK);
    fcntl(stdout_fd, F_SETFL, O_NONBLOCK);

    close(pipe_manager.get_stdin_pipe()[0]);
    close(pipe_manager.get_stdout_pipe()[1]);

    pipe_manager.get_stdin_pipe()[0] = -1;
    pipe_manager.get_stdin_pipe()[1] = -1;
    pipe_manager.get_stdout_pipe()[0] = -1;
    pipe_manager.get_stdout_pipe()[1] = -1;

    CgiProcessInfo info;
    info.pid = pid;
    info.stdin_fd = stdin_fd;
    info.stdout_fd = stdout_fd;
    return info;
}
