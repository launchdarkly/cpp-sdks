#include <sstream>

#include "context.hpp"

namespace launchdarkly {

std::string EscapeKey(std::string_view const& to_escape) {
    std::string escaped;
    for (auto& character : to_escape) {
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

std::vector<std::string_view> const& Context::kinds() {
    return kinds_;
}

Context::Context(std::string error_message)
    : errors_(error_message), valid_(false) {}

Context::Context(std::map<std::string, Attributes> attributes)
    : attributes_(std::move(attributes)), valid_(true) {
    for (auto& pair : attributes_) {
        kinds_.push_back(pair.first);
        keys_and_kinds_[pair.first] = pair.second.key();
    }

    if (keys_and_kinds_.size() == 1 && keys_and_kinds_.count("user") == 1) {
        canonical_key_ = keys_and_kinds_["user"];
    } else {
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
            if (pair.second.find_first_of("%:") != pair.second.npos) {
                std::string escaped = EscapeKey(pair.second);
                stream << pair.first << ":" << escaped;
            } else {
                stream << pair.first << ":" << pair.second;
            }
        }
        stream.flush();
        canonical_key_ = stream.str();
    }
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

std::map<std::string_view, std::string_view> Context::keys_and_kinds() const {
    return keys_and_kinds_;
}

}  // namespace launchdarkly
