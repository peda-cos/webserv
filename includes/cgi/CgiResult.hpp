#ifndef CGI_RESULT_HPP
#define CGI_RESULT_HPP

#include <string>

struct CgiResult {
    enum Status {
        SUCCESS,
        TIMEOUT,
        EXECUTION_ERROR,
        NOT_FOUND,
        NO_HANDLER
    };

    Status status;
    std::string output;

    CgiResult(Status stat, const std::string& out): status(stat), output(out) {}
};


#endif