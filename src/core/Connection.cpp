#include "../../includes/core/Connection.hpp"

Connection::Connection()
    : fd(-1), state(CONN_READING), rbuf(), wbuf(), last_activity(0),
      wants_write(false) {}

Connection::Connection(int fd)
    : fd(fd), state(CONN_READING), rbuf(), wbuf(), last_activity(0),
      wants_write(false) {}
