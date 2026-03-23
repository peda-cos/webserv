#ifndef CGI_UTILS_HPP
#define CGI_UTILS_HPP

#include <string>

class CgiUtils {
    public:
        static std::string env_normalize(const std::string& str);
};

#endif