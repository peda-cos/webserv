
#include <ParsingUtils.hpp>
#include <ConfigParser.hpp>
#include <ConfigLexer.hpp>
#include <ConfigUtils.hpp>
#include <Logger.hpp>
#include <iostream>

void testConfigParser(std::string config_source);
void testCGIExecutor();

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "config/default.conf";
    try {
        std::string config_source = ConfigUtils::get_config_content(path);
        if (config_source.empty()) {
            Logger::error("Conf file is empty in path: " + path);
            return 1;
        }
        // testConfigParser(config_source);
        testCGIExecutor();
    } catch(const std::exception& e) {
        Logger::error(e.what());
        return 1;
    }
    return 0;
}