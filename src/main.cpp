#include "../includes/core/EventLoop.hpp"
#include "../includes/core/Listener.hpp"

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  EventLoop loop;
  Listener* listener_8080 = new Listener("0.0.0.0", 8080);
  Listener* listener_8081 = new Listener("0.0.0.0", 8081);
  if (listener_8080->is_valid()) {
    loop.add_listener(listener_8080);
  } else {
    delete listener_8080;
  }
  if (listener_8081->is_valid()) {
    loop.add_listener(listener_8081);
  } else {
    delete listener_8081;
  }
  loop.run();
  return 0;
}
