#ifndef CGI_PIPE_MANAGER_HPP
#define CGI_PIPE_MANAGER_HPP

#include <string>
#include <HttpRequest.hpp>

class CgiPipeManager {
    private:
        int stdin_pipe[2];
        int stdout_pipe[2];
    public:
        CgiPipeManager();
        ~CgiPipeManager();
        void setup_child_process() const;
        void setup_parent_process() const;
        void write_to_child(const std::string& data) const;
        std::string read_from_child() const;
};

#endif
