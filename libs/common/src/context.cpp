#include <sstream>

#include "context.hpp"

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

std::vector<std::string_view> const& Context::kinds() const {
    return kinds_;
}

Context::Context(std::string error_message)
    : errors_(std::move(error_message)) {}

Context::Context(std::map<std::string, Attributes> attributes)
    : attributes_(std::move(attributes)), valid_(true) {
    for (auto& pair : attributes_) {
        kinds_.push_back(pair.first);
        keys_and_kinds_[pair.first] = pair.second.key();
    }

    canonical_key_ = make_canonical_key();
}

Value const& Context::get(std::string const& kind,
                          AttributeReference const& ref) {
    auto found = attributes_.find(kind);
    if (found != attributes_.end()) {
        return found->second.get(ref);
    }
    return Value::null();
}

Attributes const& Context::attributes(std::string const& kind) const {
    return attributes_.at(kind);
}

std::string const& Context::canonical_key() const {
    return canonical_key_;
}

std::map<std::string_view, std::string_view> const& Context::keys_and_kinds()
    const {
    return keys_and_kinds_;
}

std::string Context::make_canonical_key() {
    if (keys_and_kinds_.size() == 1) {
        if (auto iterator = keys_and_kinds_.find("user");
            iterator != keys_and_kinds_.end()) {
            return std::string(iterator->second);
        }
    }
    std::stringstream stream;
    bool first = true;
    // Maps are ordered, so keys and kinds will be in the correct order for
    // the canonical key.
    for (auto& pair : keys_and_kinds_) {
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
