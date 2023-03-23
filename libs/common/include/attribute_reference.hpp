#pragma once

#include <string>
#include <vector>

namespace launchdarkly {

class AttributeReference {
   public:
    /**
     * Get the component of the attribute reference at the specified depth.
     *
     * For example, component(1) on the reference `/a/b/c` would return
     * `b`.
     *
     * @param depth The depth to get a component for.
     * @return The component at the specified depth or an empty string if the
     * depth is out of bounds.
     */
    std::string const& component(size_t depth) const;

    /**
     * Get the total depth of the reference.
     *
     * For example, depth() on the reference `/a/b/c` would return
     * 3.
     * @return
     */
    size_t depth() const;

    /**
     * Check if the reference is a "kind" reference. Either `/kind` or `kind`.
     *
     * @return True if it is a kind reference.
     */
    bool is_kind() const;

    /** Check if the reference is valid.
     *
     * @return True if the reference is valid.
     */
    bool valid() const;

    /**
     * The redaction name will always be an attribute reference compatible
     * string. So, for instance, a literal that contained `/attr` would be
     * converted to `/~1attr`.
     * @return String to use in redacted attributes.
     */
    std::string const& redaction_name() const;

    /**
     * Create an attribute from a string that is known to be an attribute
     * reference string.
     * @param ref_str The reference string.
     * @return A new AttributeReference based on the reference string.
     */
    static AttributeReference from_reference_str(std::string ref_str);

    /**
     * Create a string from an attribute that is known to be a literal.
     *
     * This allows escaping literals that contained special characters.
     *
     * @param lit_str The literal attribute name.
     * @return A new AttributeReference based on the literal name.
     */
    static AttributeReference from_literal_str(std::string lit_str);

    bool operator==(AttributeReference const& rhs) const;
    bool operator!=(AttributeReference const& rhs) const;

   private:
    AttributeReference(std::string str, bool is_literal);

    bool valid_;

    std::string redaction_name_;
    std::vector<std::string> components_;
    inline static const std::string empty_;
};

}  // namespace launchdarkly
