#include "error.hpp"

namespace launchdarkly {

std::ostream& operator<<(std::ostream& os, Error const& e) {
    os << ErrorToString(e);
    return os;
}

char const* ErrorToString(Error e) {
    switch (e) {
        case Error::KReserved1:
            return "reserved1";
        case Error::KReserved2:
            return "reserved2";
        case Error::kConfig_Endpoints_EmptyURL:
            return "endpoints: cannot specify empty URL";
        case Error::kConfig_Endpoints_AllURLsMustBeSet:
            return "endpoints: if any endpoint is specified, then all "
                   "endpoints must be specified";
        case Error::kConfig_ApplicationInfo_EmptyKeyOrValue:
            return "application info: cannot specify an empty key or value";
        case Error::kConfig_ApplicationInfo_ValueTooLong:
            return "application info: the specified value is too long";
        case Error::kConfig_ApplicationInfo_InvalidKeyCharacters:
            return "application info: the key contains invalid characters";
        case Error::kConfig_ApplicationInfo_InvalidValueCharacters:
            return "application info: the value contains invalid characters";
        case Error::kMax:
            break;
    }
    return "unknown";
}
}  // namespace launchdarkly
