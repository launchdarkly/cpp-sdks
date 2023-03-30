#pragma once

#include <string>
#include <vector>

#include "attributes.hpp"
#include "value.hpp"

namespace launchdarkly {

class ContextBuilder;

/**
 * A LaunchDarkly context.
 */
class Context {
    friend class ContextBuilder;

   public:
    /**
     * Get the kinds the context contains.
     *
     * @return A vector of kinds.
     */
    std::vector<std::string_view> const& kinds();

    /**
     * Get a set of attributes associated with a kind.
     *
     * Only call this function if you have checked that the kind is present.
     *
     * @param kind The kind to get attributes for.
     * @return The attributes if they exist.
     */
    [[nodiscard]] Attributes const& attributes(std::string const& kind) const;

    /**
     * Get an attribute value by kind and attribute reference. If the kind is
     * not present, or the attribute not present in the kind, then
     * Value::null() will be returned.
     *
     * @param kind The kind to get the value for.
     * @param ref The reference to the desired attribute.
     * @return The attribute Value or a Value representing null.
     */
    Value const& get(std::string const& kind,
                     launchdarkly::AttributeReference const& ref);

   private:
    Context(std::map<std::string, Attributes> attributes);
    std::map<std::string, Attributes> attributes_;
    std::vector<std::string_view> kinds_;
};

}  // namespace launchdarkly
