#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>

struct Connection {
    int fd;
    std::string read_buffer;
    std::string write_buffer;

    Connection() : fd(-1) {}
    explicit Connection(int fd) : fd(fd) {}
    //sem destrutor p/ o close(fd) pois ele será gerenciado pela classe server 
};

#endif