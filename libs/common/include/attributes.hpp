#pragma once

#include <string>

#include "attribute_reference.hpp"
#include "value.hpp"

namespace launchdarkly {

/**
 * A collection of attributes that can be present within a context.
 * A multi-context has multiple sets of attributes keyed by their "kind".
 */
class Attributes {
   public:
    std::string const& key() const;

    std::string const& name() const;

    bool anonymous() const;

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
            return launchdarkly::Value::Null();
        }
        if (ref.is_kind()) {
            // Cannot access kind.
            return launchdarkly::Value::Null();
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
            return launchdarkly::Value::Null();
        } else {
            return *node;
        }
    }

    Attributes(std::string key,
               std::optional<std::string> name,
               bool anonymous,
               launchdarkly::Value attributes)
        : key_(std::move(key)),
          name_(std::move(name)),
          anonymous_(anonymous),
          custom_attributes_(std::move(attributes)) {}

   private:
    // Built-in attributes.
    launchdarkly::Value key_;
    launchdarkly::Value name_;
    launchdarkly::Value anonymous_;
    std::vector<launchdarkly::AttributeReference> private_attributes_;

    launchdarkly::Value custom_attributes_;

    // Kinds are contained at the context level, not inside attributes.
};
}  // namespace launchdarkly
