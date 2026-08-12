#include "environment_id.hpp"

namespace launchdarkly::server_side::data_components {

std::optional<std::string> EnvironmentId::Get() const {
    std::lock_guard lock(mutex_);
    return environment_id_;
}

void EnvironmentId::Set(std::string_view environment_id) {
    if (environment_id.empty()) {
        return;
    }
    std::lock_guard lock(mutex_);
    environment_id_ = std::string(environment_id);
}

}  // namespace launchdarkly::server_side::data_components
