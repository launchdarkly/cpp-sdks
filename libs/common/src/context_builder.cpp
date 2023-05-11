#include <sstream>

#include "launchdarkly/context_builder.hpp"

namespace launchdarkly {

/**
 * Verify if a kind is valid.
 * @param kind The kind to validate.
 * @return True if the kind is valid.
 */
static bool ValidKind(std::string_view kind) {
    if (kind.length() == 0) {
        return false;
    }

    return std::all_of(kind.begin(), kind.end(), [](auto character) {
        return character == '.' || character == '-' || character == '_' ||
               (character >= '0' && character <= '9') ||
               (character >= 'A' && character <= 'Z') ||
               (character >= 'a' && character <= 'z');
    });
}

AttributesBuilder<ContextBuilder, Context>& ContextBuilder::kind(
    std::string const& kind,
    std::string key) {
    auto existing = builders_.find(kind);
    if (existing != builders_.end()) {
        auto& kind_builder = builders_.at(kind);
        kind_builder.key(key);
        return kind_builder;
    }

    builders_.emplace(kind, AttributesBuilder<ContextBuilder, Context>(
                                *this, kind, std::move(key)));
    return builders_.at(kind);
}

Context ContextBuilder::build() {
    if (builders_.empty()) {
        valid_ = false;
        if (!errors_.empty()) {
            errors_.append(", ");
        }
        errors_.append(ContextErrors::kMissingKinds);
    }
    // We need to validate all the kinds. Being as kinds could be updated
    // we cannot do this validation in the `kind` method.
    for (auto& kind_builder : builders_) {
        auto const& kind = kind_builder.first;
        bool kind_valid = ValidKind(kind);
        bool key_valid = !kind_builder.second.key_.empty();
        if (!kind_valid || !key_valid) {
            valid_ = false;
            auto append = errors_.length() != 0;
            std::stringstream stream(errors_);
            if (append) {
                stream << ", ";
            }
            if (!kind_valid) {
                stream << kind << ": " << ContextErrors::kInvalidKind;
                if (!key_valid) {
                    stream << ", ";
                }
            }
            if (!key_valid) {
                stream << kind << ": " << ContextErrors::kInvalidKey;
            }
            stream.flush();
            errors_ = stream.str();
        }
    }
    if (valid_) {
        for (auto& kind_builder : builders_) {
            kinds_.emplace(kind_builder.first,
                           kind_builder.second.build_attributes());
        }
        builders_.clear();
        return {std::move(kinds_)};
    }
    return {std::move(errors_)};
}

}  // namespace launchdarkly
