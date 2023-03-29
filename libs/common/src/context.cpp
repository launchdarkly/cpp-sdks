#include "context.hpp"

namespace launchdarkly {
std::vector<std::string_view> const& Context::kinds() {
    return kinds_;
}

Context::Context(std::map<std::string, Attributes> attributes)
    : attributes_(std::move(attributes)) {
    for (auto& pair : attributes_) {
        kinds_.push_back(pair.first);
    }
}

Value const& Context::get(std::string const& kind,
                          AttributeReference const& ref) {
    auto found = attributes_.find(kind);
    if (found != attributes_.end()) {
        return found->second.get(ref);
    }
    return Value::Null();
}

Attributes const& Context::attributes(std::string const& kind) const {
    return attributes_.at(kind);
}
}  // namespace launchdarkly
