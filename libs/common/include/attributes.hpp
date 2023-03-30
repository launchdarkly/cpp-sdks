#pragma once

#include <string>
#include <unordered_set>

#include "attribute_reference.hpp"
#include "value.hpp"

namespace launchdarkly {

/**
 * A collection of attributes that can be present within a context.
 * A multi-context has multiple sets of attributes keyed by their "kind".
 */
class Attributes {
   public:
    /**
     * Get the key for the context.
     * @return A reference to the context key.
     */
    std::string const& key() const;

    /**
     * Get the name for the context.
     *
     * @return A reference to the context name, or an empty string if no name
     * is set.
     */
    std::string const& name() const;

    /**
     * Is the context anonymous or not. Defaults to false.
     * @return True if the context is anonymous.
     */
    bool anonymous() const;

    /**
     * Get a set of the private attributes for the context.
     * @return The set of private attributes for the context.
     */
    AttributeReference::SetType const& private_attributes() const {
        return private_attributes_;
    }

    /**
     * Gets the item by the specified attribute reference, or returns a null
     * Value.
     * @param ref The reference to get an attribute by.
     * @return A Value containing the requested field, or a Value representing
     * null.
     */
    launchdarkly::Value const& get(
        launchdarkly::AttributeReference const& ref) const {
        if (!ref.valid()) {
            // Cannot index by invalid references.
            return launchdarkly::Value::null();
        }
        if (ref.is_kind()) {
            // Cannot access kind.
            return launchdarkly::Value::null();
        }

        if (ref.depth() == 1) {
            // Handle built-in attributes.
            if (ref.component(0) == "key") {
                return key_;
            }
            if (ref.component(0) == "name") {
                return name_;
            }
            if (ref.component(0) == "anonymous") {
                return anonymous_;
            }
        }

        launchdarkly::Value const* node = &custom_attributes_;
        bool found = true;
        for (size_t index = 0; index < ref.depth(); index++) {
            auto const& component = ref.component(index);
            if (node->is_object()) {
                auto const& map = node->as_object();
                if (auto search = map.find(component); search != map.end()) {
                    node = &search->second;
                } else {
                    found = false;
                    break;
                }
            } else {
                found = false;
            }
        }
        if (!found) {
            return launchdarkly::Value::null();
        } else {
            return *node;
        }
    }

    /**
     * Construct a set of attributes. This is used internally by the SDK
     * but is not intended to used by consumers of the SDK.
     *
     * @param key The key for the context.
     * @param name The name of the context.
     * @param anonymous If the context is anonymous.
     * @param attributes Additional attributes for the context.
     * @param private_attributes A list of attributes that should be private.
     */
    Attributes(std::string key,
               std::optional<std::string> name,
               bool anonymous,
               launchdarkly::Value attributes,
               AttributeReference::SetType private_attributes =
                   AttributeReference::SetType())
        : key_(std::move(key)),
          name_(std::move(name)),
          anonymous_(anonymous),
          custom_attributes_(std::move(attributes)),
          private_attributes_(std::move(private_attributes)) {}

    friend std::ostream& operator<<(std::ostream& os, Attributes const& attrs) {
        os << "{key: " << attrs.key_ << ", "
           << " name: " << attrs.name_ << " anonymous: " << attrs.anonymous_
           << " private: [";
        bool first = true;
        for (auto const& private_attribute : attrs.private_attributes_) {
            if (first) {
                first = false;
            } else {
                os << ", ";
            }
            os << private_attribute;
        }
        os << "] "
           << " custom: " << attrs.custom_attributes_ << "}";

        return os;
    }

   private:
    // Built-in attributes.
    launchdarkly::Value key_;
    launchdarkly::Value name_;
    launchdarkly::Value anonymous_;
    AttributeReference::SetType private_attributes_;

    launchdarkly::Value custom_attributes_;

    // Kinds are contained at the context level, not inside attributes.
};
}  // namespace launchdarkly
