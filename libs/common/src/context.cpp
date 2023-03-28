#include "context.hpp"

#include <vector>
#include <string>

#include "attributes.hpp"
#include "value.hpp"

class Context {
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
     * @param kind The kind to get attributes for.
     * @return The attributes if they exist.
     */
    launchdarkly::Attributes const& attributes(std::string const& kind) const;

    /**
     * Get an attribute value by kind and attribute reference. If the kind is
     * not present, or the attribute not present in the kind, then
     * Value::Null() will be returned.
     *
     * @param kind The kind to get the value for.
     * @param ref The reference to the desired attribute.
     * @return The attribute Value or a Value representing null.
     */
    launchdarkly::Value const& get(std::string const& kind, launchdarkly::AttributeReference const& ref);
};
