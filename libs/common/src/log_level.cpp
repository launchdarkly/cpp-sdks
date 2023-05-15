#include <launchdarkly/logging/log_level.hpp>

#include <algorithm>
#include <string>

namespace launchdarkly {

char const* GetLogLevelName(LogLevel level, char const* default_) {
    switch (level) {
        case LogLevel::kDebug:
            return "debug";
        case LogLevel::kInfo:
            return "info";
        case LogLevel::kWarn: {
            return "warn";
            case LogLevel::kError:
                return "error";
            default:
                return default_;
        }
    }
}

LogLevel GetLogLevelEnum(char const* level, LogLevel default_) {
    if (level == nullptr) {
        return default_;
    }
    std::string lowercase = level;
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
                   tolower);

    if (lowercase == "debug") {
        return LogLevel::kDebug;
    }
    if (lowercase == "info") {
        return LogLevel::kInfo;
    }
    if (lowercase == "warn") {
        return LogLevel::kWarn;
    }
    if (lowercase == "error") {
        return LogLevel::kError;
    }
    return default_;
}

}  // namespace launchdarkly
