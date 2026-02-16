
#include "../../includes/core/EventLoop.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>

#include "../../includes/core/Connection.hpp"
#include "../../includes/core/Listener.hpp"

PollAction::PollAction() : fd(-1), events(0), remove(false) {}

PollAction::PollAction(int fd, short events, bool remove)
    : fd(fd), events(events), remove(remove) {}

EventLoop::EventLoop() : _running(false) { ::signal(SIGPIPE, SIG_IGN); }

EventLoop::~EventLoop() {
  for (std::map<int, Connection*>::iterator it = _conns.begin();
       it != _conns.end(); ++it) {
    close(it->first);
    delete it->second;
  }
  _conns.clear();
  for (std::map<int, Listener*>::iterator it = _listeners.begin();
       it != _listeners.end(); ++it) {
    delete it->second;
  }
  _listeners.clear();
}

void EventLoop::register_fd(int fd, short events, Connection* conn) {
  _pending.push_back(PollAction(fd, events, false));
  if (conn != NULL) {
    _conns[fd] = conn;
  }
}

void EventLoop::add_listener(Listener* listener) {
  if (listener == NULL) {
    return;
  }
  int fd = listener->fd();
  _listeners[fd] = listener;
  register_fd(fd, POLLIN, NULL);
}

void EventLoop::unregister_fd(int fd) {
  _pending.push_back(PollAction(fd, 0, true));
  std::map<int, Listener*>::iterator lit = _listeners.find(fd);
  if (lit != _listeners.end()) {
    delete lit->second;
    _listeners.erase(lit);
    return;
  }
  std::map<int, Connection*>::iterator it = _conns.find(fd);
  if (it != _conns.end()) {
    delete it->second;
    _conns.erase(it);
  }
  close(fd);
}

void EventLoop::modify_fd(int fd, short events) {
  _pending.push_back(PollAction(fd, events, false));
}

void EventLoop::run() {
  _running = true;
  while (_running) {
    apply_pending();
    if (_fds.empty()) {
      ::usleep(1000);
      continue;
    }
    int ready = ::poll(&_fds[0], _fds.size(), 1000);
    if (ready <= 0) {
      continue;
    }
    handle_ready();
  }
}

void EventLoop::stop() { _running = false; }

void EventLoop::apply_pending() {
  if (_pending.empty()) {
    return;
  }
  for (std::vector<PollAction>::iterator it = _pending.begin();
       it != _pending.end(); ++it) {
    if (it->remove) {
      for (std::vector<pollfd>::iterator f = _fds.begin(); f != _fds.end();
           ++f) {
        if (f->fd == it->fd) {
          _fds.erase(f);
          break;
        }
      }
      continue;
    }
    bool found = false;
    for (std::vector<pollfd>::iterator f = _fds.begin(); f != _fds.end(); ++f) {
      if (f->fd == it->fd) {
        f->events = it->events;
        found = true;
        break;
      }
    }
    if (!found) {
      pollfd pfd;
      pfd.fd = it->fd;
      pfd.events = it->events;
      pfd.revents = 0;
      _fds.push_back(pfd);
    }
  }
  _pending.clear();
}

void EventLoop::handle_ready() {
  std::vector<int> to_remove;
  for (std::vector<pollfd>::iterator it = _fds.begin(); it != _fds.end();
       ++it) {
    if (it->revents == 0) {
      continue;
    }
    if (it->revents & POLLIN) {
      std::map<int, Listener*>::iterator lit = _listeners.find(it->fd);
      if (lit != _listeners.end()) {
        int client_fd = lit->second->accept_client();
        if (client_fd >= 0) {
          int flags = ::fcntl(client_fd, F_GETFL, 0);
          if (flags != -1) {
            ::fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
          }
          ::fcntl(client_fd, F_SETFD, FD_CLOEXEC);
          Connection* conn = new Connection(client_fd);
          conn->last_activity = std::time(NULL);
          register_fd(client_fd, POLLIN, conn);
        }
        it->revents = 0;
        continue;
      }
      std::map<int, Connection*>::iterator cit = _conns.find(it->fd);
      if (cit != _conns.end()) {
        Connection* conn = cit->second;
        char buffer[8192];
        int bytes = ::recv(it->fd, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
          conn->rbuf.append(buffer, bytes);
          conn->last_activity = std::time(NULL);
          conn->state = CONN_PROCESSING;
          conn->wbuf =
              "HTTP/1.1 400 Bad Request\r\nConnection: "
              "close\r\nContent-Length: 0\r\n\r\n";
          conn->wants_write = true;
          modify_fd(it->fd, POLLIN | POLLOUT);
        } else {
          to_remove.push_back(it->fd);
        }
      }
    }
    if (it->revents & POLLOUT) {
      std::map<int, Connection*>::iterator cit = _conns.find(it->fd);
      if (cit != _conns.end()) {
        Connection* conn = cit->second;
        if (!conn->wbuf.empty()) {
          int sent = ::send(it->fd, conn->wbuf.c_str(), conn->wbuf.size(), 0);
          if (sent > 0) {
            conn->wbuf.erase(0, sent);
            conn->last_activity = std::time(NULL);
          } else {
            to_remove.push_back(it->fd);
          }
        }
        if (conn->wbuf.empty()) {
          conn->state = CONN_CLOSING;
          to_remove.push_back(it->fd);
        }
      }
    }
    if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
      to_remove.push_back(it->fd);
    }
    it->revents = 0;
  }
  for (std::vector<int>::iterator it = to_remove.begin(); it != to_remove.end();
       ++it) {
    unregister_fd(*it);
  }
}
