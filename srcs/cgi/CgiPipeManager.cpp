#include <CgiPipeManager.hpp>
#include <CgiException.hpp>

#include <unistd.h>

CgiPipeManager::CgiPipeManager()
{
    stdin_pipe[0] = -1;
    stdin_pipe[1] = -1;
    stdout_pipe[0] = -1;
    stdout_pipe[1] = -1;
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        throw CgiException("Failed to create pipes");
    }
}

CgiPipeManager::~CgiPipeManager()
{
    if (stdin_pipe[0] != -1) close(stdin_pipe[0]);
    if (stdin_pipe[1] != -1) close(stdin_pipe[1]);
    if (stdout_pipe[0] != -1) close(stdout_pipe[0]);
    if (stdout_pipe[1] != -1) close(stdout_pipe[1]);
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
