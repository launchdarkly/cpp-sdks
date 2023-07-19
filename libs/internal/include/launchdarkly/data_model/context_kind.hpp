#pragma once

#include <boost/serialization/strong_typedef.hpp>

#include <string>

namespace launchdarkly::data_model {

BOOST_STRONG_TYPEDEF(std::string, ContextKind);

inline bool IsUser(ContextKind const& kind) noexcept {
    return kind.t == "user";
}

}  // namespace launchdarkly::data_model
