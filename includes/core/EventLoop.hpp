#ifndef WEBSERV_CORE_EVENT_LOOP_HPP
#define WEBSERV_CORE_EVENT_LOOP_HPP

#include <poll.h>

#include <map>
#include <vector>

struct Connection;

struct PollAction {
  int fd;
  short events;
  bool remove;

  PollAction();
  PollAction(int fd, short events, bool remove);
};

class EventLoop {
 public:
  EventLoop();
  ~EventLoop();

  void register_fd(int fd, short events, Connection* conn);
  void unregister_fd(int fd);
  void modify_fd(int fd, short events);

  void run();
  void stop();

 private:
  std::vector<pollfd> _fds;
  std::map<int, Connection*> _conns;
  std::vector<PollAction> _pending;
  bool _running;

  void apply_pending();
  void handle_ready();
};

#endif
