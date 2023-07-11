#include <sstream>

#include <launchdarkly/context.hpp>

namespace launchdarkly {

static bool NeedsEscape(std::string_view to_check) {
    return to_check.find_first_of("%:") != std::string_view::npos;
}

static std::string EscapeKey(std::string_view const& to_escape) {
    std::string escaped;
    for (auto const& character : to_escape) {
        if (character == '%') {
            escaped.append("%3A");
        } else if (character == ':') {
            escaped.append("%25");
        } else {
            escaped.push_back(character);
        }
    }
    return escaped;
}

std::vector<std::string> const& Context::Kinds() const {
    return kinds_;
}

Context::Context(std::string error_message)
    : errors_(std::move(error_message)) {}

Context::Context(std::map<std::string, launchdarkly::Attributes> attributes)
    : attributes_(std::move(attributes)), valid_(true) {
    for (auto& pair : attributes_) {
        kinds_.push_back(pair.first);
        kinds_to_keys_[pair.first] = pair.second.Key();
    }

    canonical_key_ = make_canonical_key();
}

Value const& Context::Get(std::string const& kind,
                          AttributeReference const& ref) const {
    auto found = attributes_.find(kind);
    if (found != attributes_.end()) {
        return found->second.Get(ref);
    }
    return Value::Null();
}

Attributes const& Context::Attributes(std::string const& kind) const {
    return attributes_.at(kind);
}

std::string const& Context::CanonicalKey() const {
    return canonical_key_;
}

std::map<std::string, std::string> const& Context::KindsToKeys() const {
    return kinds_to_keys_;
}

std::string Context::make_canonical_key() {
    if (kinds_to_keys_.size() == 1) {
        if (auto iterator = kinds_to_keys_.find("user");
            iterator != kinds_to_keys_.end()) {
            return iterator->second;
        }
    }
    std::stringstream stream;
    bool first = true;
    // Maps are ordered, so keys and kinds will be in the correct order for
    // the canonical key.
    for (auto& pair : kinds_to_keys_) {
        if (first) {
            first = false;
        } else {
            stream << ":";
        }
        if (NeedsEscape(pair.second)) {
            std::string escaped = EscapeKey(pair.second);
            stream << pair.first << ":" << escaped;
        } else {
            stream << pair.first << ":" << pair.second;
        }
    }
    stream.flush();
    return stream.str();
}

}  // namespace launchdarkly
