#include "../includes/core/EventLoop.hpp"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    EventLoop loop;
    loop.run();
    return 0;
}
