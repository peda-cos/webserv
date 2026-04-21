#ifndef MIME_TYPES_HPP
#define MIME_TYPES_HPP

#include <string>

class MimeTypes {
    public:
        static std::string getType(const std::string& filepath);

    private:
        static std::string _getExtension(const std::string& filepath);
};

#endif
