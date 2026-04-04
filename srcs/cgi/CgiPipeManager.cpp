#include <CgiPipeManager.hpp>
#include <CgiException.hpp>

#include <unistd.h>
#include <sys/select.h>
#include <Constants.hpp>
#include <signal.h>
#include <sys/wait.h>

CgiPipeManager::CgiPipeManager()
{
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        throw CgiException("Failed to create pipes");
    }
}

CgiPipeManager::~CgiPipeManager()
{
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
}

void CgiPipeManager::setup_child_process() const
{
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    close(stdin_pipe[0]);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
}

void CgiPipeManager::setup_parent_process() const
{
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
}

void CgiPipeManager::write_to_child(const std::string& data) const
{
    if (!data.empty()) {
        write(stdin_pipe[1], data.c_str(), data.size());
    }
    close(stdin_pipe[1]);
}

int CgiPipeManager::handle_timeout() const
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(stdout_pipe[0], &read_fds);

    struct timeval timeout;
    timeout.tv_sec = CGI_TIMEOUT_SECONDS;
    timeout.tv_usec = 0;

    return select(stdout_pipe[0] + 1, &read_fds, NULL, NULL, &timeout);
}

std::string CgiPipeManager::read_from_child(pid_t pid) const
{
    std::string result = "";
    char buffer[1024];
    while (true) {

        int timeout_result = handle_timeout();
        if (timeout_result == 0) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            throw CgiException("CGI script execution timed out");
        } else if (timeout_result == -1) {
            throw CgiException("select() error in timeout handling");
        }
        ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0) break;
        buffer[bytesRead] = '\0';
        result.append(buffer);
    }
    close(stdout_pipe[0]);
    return result;
}