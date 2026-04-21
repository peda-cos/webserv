#include <MimeTypes.hpp>

std::string MimeTypes::_getExtension(const std::string& filepath)
{
    std::string::size_type dot = filepath.rfind('.');
    if (dot == std::string::npos || dot == filepath.size() - 1) {
        return "";
    }
    return filepath.substr(dot + 1);
}

std::string MimeTypes::getType(const std::string& filepath)
{
    std::string ext = _getExtension(filepath);

    if (ext == "html" || ext == "htm")  return "text/html";
    if (ext == "css")                   return "text/css";
    if (ext == "js")                    return "application/javascript";
    if (ext == "json")                  return "application/json";
    if (ext == "txt")                   return "text/plain";
    if (ext == "png")                   return "image/png";
    if (ext == "jpg" || ext == "jpeg")  return "image/jpeg";
    if (ext == "gif")                   return "image/gif";
    if (ext == "ico")                   return "image/x-icon";
    if (ext == "svg")                   return "image/svg+xml";
    if (ext == "pdf")                   return "application/pdf";
    if (ext == "xml")                   return "application/xml";

    return "application/octet-stream";
}
