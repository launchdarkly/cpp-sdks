#include "launchdarkly/client_side/api.hpp"

#include <cstdint>
#include <optional>

namespace launchdarkly {

auto const kAnswerToLifeTheUniverseAndEverything = 42;

std::optional<std::int32_t> foo() {
    return kAnswerToLifeTheUniverseAndEverything;
}
}  // namespace launchdarkly
