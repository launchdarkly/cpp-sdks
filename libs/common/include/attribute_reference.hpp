#pragma once

#include <ostream>
#include <string>
<<<<<<< HEAD
#include <unordered_set>
#include <vector>

#include <boost/functional/hash.hpp>

=======
#include <vector>

>>>>>>> main
namespace launchdarkly {

/**
 *  Represents an attribute name or path expression identifying a value within a
 * [TODO: Context]. This can be used to retrieve a value with [TODO: Get Value],
 * or to identify an attribute or nested value that should be considered private
 * with [TODO: private attribute] (the SDK configuration can also have a list of
 * private attribute references).
 *
 *  This is represented as a separate type, rather than just a string, so that
 * validation and parsing can be done ahead of time if an attribute reference
 * will be used repeatedly later (such as in flag evaluations).
 *
 *  If the string starts with '/', then this is treated as a slash-delimited
 * path reference where the first component is the name of an attribute, and
 * subsequent components are the names of nested JSON object properties. In this
 * syntax, the escape sequences "~0" and "~1" represent '~' and '/' respectively
 * within a path component.
 *
 *  If the string does not start with '/', then it is treated as the literal
 * name of an attribute.
 */
class AttributeReference {
   public:
    /**
<<<<<<< HEAD
     * Provides a hashing function for use with unordered sets.
     */
    struct HashFunction {
        size_t operator()(AttributeReference const& ref) const {
            return boost::hash_range(ref.components_.begin(),
                                     ref.components_.end());
        }
    };

    using SetType = std::unordered_set<AttributeReference,
                                       AttributeReference::HashFunction>;

    /**
=======
>>>>>>> main
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
     * For example, depth() on the reference `/a/b/c` would return 3.
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

    friend std::ostream& operator<<(std::ostream& os,
                                    AttributeReference const& ref) {
        os << (ref.valid() ? "valid" : "invalid") << "(" << ref.redaction_name()
           << ")";
        return os;
    }

<<<<<<< HEAD
    /**
     * Construct an attribute reference from a string.
     * @param refStr The string to make an attribute reference from.
     */
    AttributeReference(std::string refStr);

    /**
     * Construct an attribute reference from a constant string.
     * @param refStr The string to make an attribute reference from.
     */
    AttributeReference(char const* refStr);

    bool operator==(AttributeReference const& other) const {
        return components_ == other.components_;
    }

    bool operator!=(AttributeReference const& other) const {
        return !(*this == other);
    }

    bool operator<(AttributeReference const& rhs) const {
        return components_ < rhs.components_;
    }

=======
>>>>>>> main
   private:
    AttributeReference(std::string str, bool is_literal);

    bool valid_;

    std::string redaction_name_;
    std::vector<std::string> components_;
    inline static const std::string empty_;
};

}  // namespace launchdarkly
