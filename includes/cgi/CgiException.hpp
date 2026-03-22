#ifndef CGI_EXCEPTION_HPP
#define CGI_EXCEPTION_HPP

#include <exception>
#include <string>

class CgiException : public std::exception {
    private:
        std::string message;
    public:
        explicit CgiException(const std::string& msg) : message(msg) {}
        virtual ~CgiException() throw() {}
        virtual const char* what() const throw() {
            return message.c_str();
        }
};

#endif