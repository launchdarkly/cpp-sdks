#include <launchdarkly/error.hpp>

namespace launchdarkly {
std::ostream& operator<<(std::ostream& os, Error const& err) {
    os << ErrorToString(err);
    return os;
}

char const* ErrorCodeToString(ErrorCode const& err);

template <class>
inline constexpr bool always_false_v = false;

char const* ErrorToString(Error const& err) {
    return std::visit(
        [](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ErrorCode>) {
                return ErrorCodeToString(arg);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg.c_str();
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
            }
        }, err);
}

char const* ErrorCodeToString(ErrorCode const& err) {
    switch (err) {
        case ErrorCode::KReserved1:
            return "reserved1";
        case ErrorCode::KReserved2:
            return "reserved2";
        case ErrorCode::kConfig_Endpoints_EmptyURL:
            return "endpoints: cannot specify empty URL";
        case ErrorCode::kConfig_Endpoints_AllURLsMustBeSet:
            return "endpoints: if any endpoint is specified, then all "
                "endpoints must be specified";
        case ErrorCode::kConfig_ApplicationInfo_EmptyKeyOrValue:
            return "application info: cannot specify an empty key or value";
        case ErrorCode::kConfig_ApplicationInfo_ValueTooLong:
            return "application info: the specified value is too long";
        case ErrorCode::kConfig_ApplicationInfo_InvalidKeyCharacters:
            return "application info: the key contains invalid characters";
        case ErrorCode::kConfig_ApplicationInfo_InvalidValueCharacters:
            return "application info: the value contains invalid characters";
        case ErrorCode::kConfig_Events_ZeroCapacity:
            return "events: capacity must be non-zero";
        case ErrorCode::kConfig_SDKKey_Empty:
            return "sdk key: cannot be empty";
        case ErrorCode::kConfig_DataSystem_LazyLoad_MissingSource:
            return "data system: lazy load config requires a source";
        case ErrorCode::kMax:
            break;
    }
    return "unknown";
}

bool operator==(Error const& lhs, ErrorCode const& rhs) {
    if (auto const* err = std::get_if<ErrorCode>(&lhs)) {
        return *err == rhs;
    }
    return false;
}
} // namespace launchdarkly
