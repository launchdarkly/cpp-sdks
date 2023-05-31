#include <sstream>

#include <launchdarkly/context_builder.hpp>

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

    if (kind == "multi" || kind == "kind") {
        return false;
    }

    return std::all_of(kind.begin(), kind.end(), [](auto character) {
        return character == '.' || character == '-' || character == '_' ||
               (character >= '0' && character <= '9') ||
               (character >= 'A' && character <= 'Z') ||
               (character >= 'a' && character <= 'z');
    });
}

ContextBuilder::ContextBuilder(Context const& context) {
    if (!context.Valid()) {
        return;
    }

    for (auto& kind : context.kinds_) {
        auto& attributes = context.Attributes(kind);
        builders_.emplace(kind, AttributesBuilder<ContextBuilder, Context>(
                                    *this, kind, attributes));
    }
}

AttributesBuilder<ContextBuilder, Context>& ContextBuilder::Kind(
    std::string const& kind,
    std::string key) {
    auto existing = builders_.find(kind);
    if (existing != builders_.end()) {
        auto& kind_builder = builders_.at(kind);
        kind_builder.Key(key);
        return kind_builder;
    }

    builders_.emplace(kind, AttributesBuilder<ContextBuilder, Context>(
                                *this, kind, std::move(key)));
    return builders_.at(kind);
}

Context ContextBuilder::Build() const {
    bool valid = true;
    std::string errors;
    std::map<std::string, Attributes> kinds;

    if (builders_.empty()) {
        valid = false;
        if (!errors.empty()) {
            errors.append(", ");
        }
        errors.append(ContextErrors::kMissingKinds);
    }
    // We need to validate all the kinds. Being as kinds could be updated
    // we cannot do this validation in the `kind` method.
    for (auto& kind_builder : builders_) {
        auto const& kind = kind_builder.first;
        bool kind_valid = ValidKind(kind);
        bool key_valid = !kind_builder.second.key_.empty();
        if (!kind_valid || !key_valid) {
            valid = false;
            auto append = errors.length() != 0;
            std::stringstream stream(errors);
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
            errors = stream.str();
        }
    }
    if (valid) {
        for (auto& kind_builder : builders_) {
            kinds.emplace(kind_builder.first,
                          kind_builder.second.BuildAttributes());
        }
        return {std::move(kinds)};
    }
    return {std::move(errors)};
}

AttributesBuilder<ContextBuilder, Context>* ContextBuilder::Kind(
    std::string kind) {
    if (builders_.count(kind)) {
        return &builders_.at(kind);
    }
    return nullptr;
}

}  // namespace launchdarkly
