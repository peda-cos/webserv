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

std::string CgiExecutor::execute(const HttpRequest& request, char** envp)
{
    CgiPipeManager pipe;

    pid = fork();
    if (pid == -1) {
        throw CgiException("Failed to fork process");
    }
    if (pid == 0) {
        pipe.setup_child_process();
        
        // TODO: configurar o argumento do script dinamicamente
        const char* script_path = "/tmp/test_cgi_script.py"; 
        const char* args[] = { script_path, NULL };
        
        execve(script_path, (char* const*)args, envp);
        
        Logger::error("Execve failed: " + std::string(strerror(errno)));
        exit(1);
    } else {
        pipe.setup_parent_process();
        std::string data = request.method == POST ? request.body : "";
        pipe.write_to_child(data);

        int status;
        waitpid(pid, &status, 0);
        std::string output = pipe.read_from_child();

        // TODO: adicionar tratamento de resposta CGI (Como vai se integrar ao Http?)
        // Retornar um objeto com flag de error e body, ou vai lancar exception? E o HTTP usa try/catch?
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                Logger::error("CGI script exited with code: " + StringUtils::to_string(WEXITSTATUS(status)));
                return "Error executing CGI script";
            } else {
                Logger::info("CGI script executed successfully");
            }
        } 
        return output;
    }
    return "CGI script executed successfully!";
}
