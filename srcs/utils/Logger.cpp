#include "Logger.hpp"

#include <iostream>

void Logger::info(const std::string& message) {
    std::cout << BLUE_COLOR << "[ INFO ] " << RESET_COLOR
        << message << std::endl;
}

void Logger::warn(const std::string& message) {
    std::cout << YELLOW_COLOR << "[ WARNING ] " << RESET_COLOR
        << "Attention " << message << std::endl;
}

void Logger::debug(const std::string& message) {
    std::cout << CYAN_COLOR << "[ DEBUG ] " << RESET_COLOR
        << message << std::endl;
}

void Logger::error(const std::string& message) {
    std::cerr << RED_COLOR << "[ ERROR ] " << RESET_COLOR
        << "Problem in " << message << std::endl;
}