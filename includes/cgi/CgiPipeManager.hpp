#ifndef CGI_PIPE_MANAGER_HPP
#define CGI_PIPE_MANAGER_HPP

#include <string>
#include <sys/types.h>

class CgiPipeManager {
    private:
        int stdin_pipe[2];
        int stdout_pipe[2];
    public:
        CgiPipeManager();
        ~CgiPipeManager();
        void setup_child_process() const;
        int* get_stdin_pipe() { return stdin_pipe; }
        int* get_stdout_pipe() { return stdout_pipe; }
};

#endif
