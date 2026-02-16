#include "../../includes/core/Listener.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

Listener::Listener(const std::string& host, int port)
    : _fd(-1), _host(host), _port(port) {
  _fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_fd < 0) {
    return;
  }
  int opt = 1;
  ::setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  int flags = ::fcntl(_fd, F_GETFL, 0);
  if (flags != -1) {
    ::fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
  }
  ::fcntl(_fd, F_SETFD, FD_CLOEXEC);

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<unsigned short>(_port));
  if (_host.empty() || _host == "0.0.0.0") {
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    addr.sin_addr.s_addr = ::inet_addr(_host.c_str());
  }
  if (::bind(_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(_fd);
    _fd = -1;
    return;
  }
  if (::listen(_fd, 128) != 0) {
    ::close(_fd);
    _fd = -1;
    return;
  }
}

Listener::~Listener() {
  if (_fd >= 0) {
    ::close(_fd);
  }
}

int Listener::fd() const { return _fd; }

bool Listener::is_valid() const { return _fd >= 0; }

int Listener::accept_client() const {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  return ::accept(_fd, reinterpret_cast<sockaddr*>(&addr), &len);
}
