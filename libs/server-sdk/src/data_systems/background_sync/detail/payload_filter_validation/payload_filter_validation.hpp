#pragma once

#include <string>

namespace launchdarkly::server_side::data_systems::detail {
bool ValidateFilterKey(std::string const& filter_key);
}
