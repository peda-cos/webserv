#ifndef VALIDATION_ERROR_HPP
#define VALIDATION_ERROR_HPP

#include <string>
#include <sstream>

enum ErrorSeverity {
    SEVERITY_ERROR,
    SEVERITY_WARNING
};

enum ErrorStage {
    STAGE_LEXER,
    STAGE_PARSER,
    STAGE_VALIDATOR
};

struct ValidationError {
    ErrorStage stage;
    ErrorSeverity severity;
    size_t line;
    std::string message;
    std::string context;  // Ex: "server { listen ... }"

    ValidationError(ErrorStage st, ErrorSeverity sev, size_t ln,
                    const std::string& msg, const std::string& ctx = "")
        : stage(st), severity(sev), line(ln), message(msg), context(ctx) {}

    std::string toString() const {
        std::stringstream ss;
        ss << "[" << stageToString(stage) << "] "
           << severityToString(severity) << " at line " << line
           << ": " << message;
        if (!context.empty()) {
            ss << " (context: " << context << ")";
        }
        return ss.str();
    }

private:
    static const char* stageToString(ErrorStage s) {
        switch (s) {
            case STAGE_LEXER: return "LEXER";
            case STAGE_PARSER: return "PARSER";
            case STAGE_VALIDATOR: return "VALIDATOR";
            default: return "UNKNOWN";
        }
    }

    static const char* severityToString(ErrorSeverity s) {
        switch (s) {
            case SEVERITY_ERROR: return "ERROR";
            case SEVERITY_WARNING: return "WARNING";
            default: return "UNKNOWN";
        }
    }
};

#endif
