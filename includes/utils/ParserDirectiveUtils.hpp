#ifndef PARSER_DIRECTIVE_UTILS_HPP
#define PARSER_DIRECTIVE_UTILS_HPP

#include <string>
#include <vector>
#include <cstddef>
#include <Enums.hpp>

namespace ParserDirectiveUtils {
    bool is_digits_only(const std::string& value);
    bool is_valid_ipv4(const std::string& host);
    bool is_valid_hostname_label(const std::string& label);
    bool is_valid_hostname(const std::string& host);
    bool is_valid_listen_host(const std::string& host);

    bool parse_port(const std::string& value, int& port);
    bool parse_body_size(const std::string& raw, size_t& out_value);

    bool is_valid_error_status_code(int status_code);
    bool is_valid_error_page_path(const std::string& path);
    bool is_valid_redirect_status_code(int status_code);
    bool is_valid_redirect_target(const std::string& target);

    bool is_valid_cgi_extension(const std::string& extension);
    bool is_valid_cgi_handler_path(const std::string& path);

    bool has_method(const std::vector<HttpMethod>& methods, HttpMethod method);
}

#endif