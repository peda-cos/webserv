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

// TODO: Nos returns do metodo onde deveria ocorrer falha, como tratar?
// Lançar exception? E o HTTP usa try/catch? 
// Como vai se integrar ao Http?
// Retornar um objeto com flag de error e body, ou vai lancar exception? E o HTTP usa try/catch?

std::string CgiExecutor::execute(const HttpRequest& request, LocationConfig& location_config)
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
        throw CgiException("No CGI handler configured for extension: " + file_extension);
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw CgiException("Failed to fork process");
    }
    if (pid == 0) {
        pipe.setup_child_process();
        const char* args[] = { cgi_interpreter.c_str(), cgi_script_path.c_str(), NULL };
        CgiEnvBuilder env_builder(request);
        execve(args[0], (char* const*)args, env_builder.getEnvp());

        // execve failed, exit with error code
        exit(127);
    }
    pipe.setup_parent_process();
    std::string data = request.method == "POST" ? request.body : "";
    pipe.write_to_child(data);

    std::string output = "";
    try {
        output = pipe.read_from_child(pid);
    } catch(const CgiException& e) {
        throw;
    }

    int status;
    waitpid(pid, &status, 0);
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw CgiException("CGI script exited with code: " + StringUtils::to_string(WEXITSTATUS(status)));
    }
    Logger::info("CGI script executed successfully");
    return output;
}
