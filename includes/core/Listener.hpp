#ifndef WEBSERV_CORE_LISTENER_HPP
#define WEBSERV_CORE_LISTENER_HPP

#include <string>

class Listener {
 public:
  Listener(const std::string& host, int port);
  ~Listener();

  int fd() const;
  bool is_valid() const;
  int accept_client() const;

 private:
  int _fd;
  std::string _host;
  int _port;
};

#endif
