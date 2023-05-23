#pragma once

#include <string>
#include <vector>

#include <launchdarkly/attributes.hpp>
#include <launchdarkly/value.hpp>

namespace launchdarkly {

struct ContextErrors {
    inline static const std::string kInvalidKind =
        "\"Kind contained invalid characters. A kind may contain ASCII letters "
        "or "
        "numbers, as well as '.', '-', and '_'.\"";
    // For now disallow an empty key. This may need changed if anonymous key
    // generation and persistence is added.
    inline static const std::string kInvalidKey =
        "\"The key for a context may not be empty.\"";
    inline static const std::string kMissingKinds =
        "\"The context must contain at least 1 kind.\"";
};

class ContextBuilder;

/**
 * A LaunchDarkly context.
 */
class Context final {
    friend class ContextBuilder;

   public:
    /**
     * Get the kinds the context contains.
     *
     * @return A vector of kinds.
     */
    [[nodiscard]] std::vector<std::string> const& Kinds() const;

    /**
     * Get a set of attributes associated with a kind.
     *
     * Only call this function if you have checked that the kind is present.
     *
     * @param kind The kind to get attributes for.
     * @return The attributes if they exist.
     */
    [[nodiscard]] launchdarkly::Attributes const& Attributes(
        std::string const& kind) const;

    /**
     * Get an attribute value by kind and attribute reference. If the kind is
     * not present, or the attribute not present in the kind, then
     * Value::null() will be returned.
     *
     * @param kind The kind to get the value for.
     * @param ref The reference to the desired attribute.
     * @return The attribute Value or a Value representing null.
     */
    Value const& Get(std::string const& kind,
                     launchdarkly::AttributeReference const& ref);

    /**
     * Check if a context is valid.
     *
     * @return Returns true if the context is valid.
     */
    [[nodiscard]] bool Valid() const { return valid_; }

    /**
     * Get the canonical key for this context.
     */
    [[nodiscard]] std::string const& CanonicalKey() const;

    /**
     * Get a collection containing the kinds and their associated keys.
     *
     * @return Returns a map of kinds to keys.
     */
    [[nodiscard]] std::map<std::string, std::string> const& KindsToKeys() const;

    /**
     * Get a string containing errors the context encountered during
     * construction.
     *
     * @return A string containing errors, or an empty string if there are no
     * errors.
     */
    std::string const& errors() { return errors_; }

    friend std::ostream& operator<<(std::ostream& out, Context const& context) {
        if (context.valid_) {
            out << "{contexts: [";
            bool first = true;
            for (auto const& kind : context.attributes_) {
                if (first) {
                    first = false;
                } else {
                    out << ", ";
                }
                out << "kind: " << kind.first << " attributes: " << kind.second;
            }
            out << "]";
        } else {
            out << "{invalid: errors: [" << context.errors_ << "]";
        }

        return out;
    }

    ~Context() = default;
    Context(Context const& context) = default;
    Context(Context&& context) = default;
    Context& operator=(Context const&) = default;
    Context& operator=(Context&&) = default;

   private:
    /**
     * Create an invalid context with the given error message.
     */
    Context(std::string error_message);

    /**
     * Create a valid context with the given attributes.
     * @param attributes
     */
    Context(std::map<std::string, launchdarkly::Attributes> attributes);

    std::map<std::string, launchdarkly::Attributes> attributes_;
    std::vector<std::string> kinds_;
    std::map<std::string, std::string> kinds_to_keys_;
    bool valid_ = false;
    std::string errors_;
    std::string canonical_key_;

    std::string make_canonical_key();
};

}  // namespace launchdarkly
