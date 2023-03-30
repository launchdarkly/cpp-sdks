#include <sstream>

#include "context_builder.hpp"

namespace launchdarkly {

/**
 * Verify if a kind is valid.
 * @param kind The kind to validate.
 * @return True if the kind is valid.
 */
static bool ValidKind(std::string kind) {
    if (kind.length() == 0) {
        return false;
    }
    for (auto const& character : kind) {
        auto valid = character == '.' || character == '-' || character == '_' ||
                     (character >= '0' && character <= '9') ||
                     (character >= 'A' && character <= 'Z') ||
                     (character >= 'a' && character <= 'z');
        if (!valid) {
            return false;
        }
    }
    // Did not encounter any validation errors.
    return true;
}

AttributesBuilder<ContextBuilder, Context> ContextBuilder::kind(
    std::string kind,
    std::string key) {
    auto kind_valid = ValidKind(kind);
    auto key_valid = key != "";
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
    return {*this, std::move(kind), std::move(key)};
}

Context ContextBuilder::build() {
    if (valid_) {
        return {kinds_};
    } else {
        return {errors_};
    }
}

void ContextBuilder::internal_add_kind(std::string kind, Attributes attrs) {
    kinds_.insert_or_assign(std::move(kind), std::move(attrs));
}

}  // namespace launchdarkly
