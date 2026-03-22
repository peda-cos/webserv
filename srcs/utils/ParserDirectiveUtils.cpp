#include <sstream>
#include <climits>
#include <cctype>
#include <limits>

#include <ParsingUtils.hpp>
#include <ParserDirectiveUtils.hpp>

namespace ParserDirectiveUtils {
    bool is_digits_only(const std::string& value) {
        if (value.empty())
            return false;
        for (size_t i = 0; i < value.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(value[i])))
                return false;
        }
        return true;
    }

    bool is_valid_ipv4(const std::string& host) {
        std::vector<std::string> octets = ParsingUtils::split(host, '.');
        if (octets.size() != 4)
            return false;
        for (size_t i = 0; i < octets.size(); ++i) {
            if (!is_digits_only(octets[i]))
                return false;
            std::stringstream ss(octets[i]);
            int value = -1;
            if (!(ss >> value) || !ss.eof() || value < 0 || value > 255)
                return false;
        }
        return true;
    }

    bool is_valid_hostname_label(const std::string& label) {
        if (label.empty() || label.size() > 63)
            return false;
        if (!std::isalnum(static_cast<unsigned char>(label[0]))
            || !std::isalnum(static_cast<unsigned char>(label[label.size() - 1]))) {
            return false;
        }
        for (size_t i = 0; i < label.size(); ++i) {
            char c = label[i];
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-')
                return false;
        }
        return true;
    }

    bool is_valid_hostname(const std::string& host) {
        if (host.empty() || host.size() > 253)
            return false;
        if (host == "localhost")
            return true;

        std::string candidate = host;
        if (candidate.size() > 2 && candidate[0] == '*' && candidate[1] == '.')
            candidate = candidate.substr(2);

        std::vector<std::string> labels = ParsingUtils::split(candidate, '.');
        if (labels.empty())
            return false;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (!is_valid_hostname_label(labels[i]))
                return false;
        }
        return true;
    }

    bool is_valid_listen_host(const std::string& host) {
        if (is_valid_ipv4(host))
            return true;
        
        std::vector<std::string> parts = ParsingUtils::split(host, '.');
        if (parts.size() == 4) {
            bool all_digits = true;
            for (size_t i = 0; i < parts.size(); ++i) {
                if (!is_digits_only(parts[i])) {
                    all_digits = false;
                    break;
                }
            }
            if (all_digits)
                return false;
        }
        
        return is_valid_hostname(host);
    }

    bool parse_port(const std::string& value, int& port) {
        if (!is_digits_only(value))
            return false;
        std::stringstream ss(value);
        if (!(ss >> port) || !ss.eof())
            return false;
        return port >= 1 && port <= 65535;
    }

    bool parse_body_size(const std::string& raw, size_t& out_value) {
        if (raw.empty())
            return false;

        std::string number_part = raw;
        unsigned long long multiplier = 1;

        char suffix = raw[raw.size() - 1];
        if (std::isalpha(static_cast<unsigned char>(suffix))) {
            number_part = raw.substr(0, raw.size() - 1);
            if (number_part.empty())
                return false;
            if (suffix == 'K' || suffix == 'k') multiplier = 1024ULL;
            else if (suffix == 'M' || suffix == 'm') multiplier = 1024ULL * 1024ULL;
            else if (suffix == 'G' || suffix == 'g') multiplier = 1024ULL * 1024ULL * 1024ULL;
            else return false;
        }

        if (!is_digits_only(number_part))
            return false;

        std::stringstream ss(number_part);
        unsigned long long base = 0;
        if (!(ss >> base) || !ss.eof())
            return false;
        if (base == 0)
            return false;
        if (base > ULLONG_MAX / multiplier)
            return false;

        unsigned long long calculated = base * multiplier;
        if (calculated > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
            return false;
        out_value = static_cast<size_t>(calculated);
        return true;
    }

    bool is_valid_error_status_code(int status_code) {
        return status_code >= 300 && status_code <= 599;
    }

    bool is_valid_error_page_path(const std::string& path) {
        if (path.empty())
            return false;
        if (path[0] == '/' || path[0] == '@')
            return true;
        return path.find("http://") == 0 || path.find("https://") == 0;
    }

    bool is_valid_redirect_status_code(int status_code) {
        return status_code >= 300 && status_code <= 399;
    }

    bool is_valid_redirect_target(const std::string& target) {
        if (target.empty())
            return false;
        if (target[0] == '/')
            return true;
        return target.find("http://") == 0 || target.find("https://") == 0;
    }

    bool has_method(const std::vector<HttpMethod>& methods, HttpMethod method) {
        for (size_t i = 0; i < methods.size(); ++i) {
            if (methods[i] == method)
                return true;
        }
        return false;
    }
}