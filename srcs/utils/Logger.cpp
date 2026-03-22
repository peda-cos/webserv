#include "Logger.hpp"

#include <iostream>

namespace Logger {
void info(const std::string& message) {
    std::cout << BLUE_COLOR << "[ INFO ] " << RESET_COLOR
        << message << std::endl;
}

void warn(const std::string& message) {
    std::cout << YELLOW_COLOR << "[ WARNING ] " << RESET_COLOR
        << message << std::endl;
}

void debug(const std::string& message) {
    std::cout << CYAN_COLOR << "[ DEBUG ] "<< RESET_COLOR
        << message << std::endl;
}

void error(const std::string& message) {
    std::cerr << RED_COLOR << "[ ERROR ] " << RESET_COLOR
        << message << std::endl;
}
}