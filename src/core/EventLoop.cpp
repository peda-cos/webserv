#include "../../includes/core/EventLoop.hpp"
#include "../../includes/core/Connection.hpp"

#include <signal.h>
#include <unistd.h>

PollAction::PollAction() : fd(-1), events(0), remove(false) {}

PollAction::PollAction(int fd, short events, bool remove)
    : fd(fd), events(events), remove(remove) {}

EventLoop::EventLoop() : _running(false) {
    ::signal(SIGPIPE, SIG_IGN);
}

EventLoop::~EventLoop() {
    for (std::map<int, Connection *>::iterator it = _conns.begin();
         it != _conns.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    _conns.clear();
}

void EventLoop::register_fd(int fd, short events, Connection *conn) {
    _pending.push_back(PollAction(fd, events, false));
    if (conn != NULL) {
        _conns[fd] = conn;
    }
}

void EventLoop::unregister_fd(int fd) {
    _pending.push_back(PollAction(fd, 0, true));
    std::map<int, Connection *>::iterator it = _conns.find(fd);
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

void EventLoop::stop() {
    _running = false;
}

void EventLoop::apply_pending() {
    if (_pending.empty()) {
        return;
    }
    for (std::vector<PollAction>::iterator it = _pending.begin();
         it != _pending.end(); ++it) {
        if (it->remove) {
            for (std::vector<pollfd>::iterator f = _fds.begin();
                 f != _fds.end(); ++f) {
                if (f->fd == it->fd) {
                    _fds.erase(f);
                    break;
                }
            }
            continue;
        }
        bool found = false;
        for (std::vector<pollfd>::iterator f = _fds.begin();
             f != _fds.end(); ++f) {
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
        }
        if (it->revents & (POLLHUP | POLLERR | POLLNVAL)) {
            to_remove.push_back(it->fd);
        }
        it->revents = 0;
    }
    for (std::vector<int>::iterator it = to_remove.begin();
         it != to_remove.end(); ++it) {
        unregister_fd(*it);
    }
}
