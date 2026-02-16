#ifndef WEBSERV_CORE_CONNECTION_HPP
#define WEBSERV_CORE_CONNECTION_HPP

#include <ctime>
#include <string>

enum ConnState { CONN_READING, CONN_PROCESSING, CONN_WRITING, CONN_CLOSING };

struct Connection {
  int fd;
  ConnState state;
  std::string rbuf;
  std::string wbuf;
  std::time_t last_activity;
  bool wants_write;

  Connection();
  explicit Connection(int fd);
};

#endif
