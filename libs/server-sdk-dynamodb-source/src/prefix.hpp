#pragma once

#include <string>

namespace launchdarkly::server_side::integrations::detail {

inline std::string PrefixedNamespace(std::string const& prefix,
                                     std::string const& base) {
    if (prefix.empty()) {
        return base;
    }
    return prefix + ":" + base;
}

}  // namespace launchdarkly::server_side::integrations::detail
