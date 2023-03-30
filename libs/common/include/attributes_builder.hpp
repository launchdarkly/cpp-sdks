#pragma once

#include <string>

#include "attribute_reference.hpp"
#include "attributes.hpp"
#include "value.hpp"

namespace launchdarkly {

/**
 * This is used in the implementation of the context builder for setting
 * attributes for a single context. This is not intended to be directly
 * used by an SDK consumer.
 *
 * @tparam BuilderReturn The type of builder using the AttributesBuilder.
 * @tparam BuildType The type of object being built.
 */
template <class BuilderReturn, class BuildType>
class AttributesBuilder {
   public:
    /**
     * Create an attributes builder with the given key.
     * @param key A unique string identifying a context.
     */
    AttributesBuilder(BuilderReturn& builder, std::string kind, std::string key)
        : key_(std::move(key)), kind_(std::move(kind)), builder_(builder) {}

    /**
     * The context's name.
     *
     * You can search for contexts on the Contexts page by name.
     *
     * @param name
     * @return A reference to the current builder.
     */
    AttributesBuilder& name(std::string name);

    /**
     * If true, the context will _not_ appear on the Contexts page in the
     * LaunchDarkly dashboard.
     *
     * @param anonymous The value to set.
     * @return A reference to the current builder.
     */
    AttributesBuilder& anonymous(bool anonymous);

    /**
     * Add or update an attribute in the context.
     *
     * This method cannot be used to set the key, kind, name, or anonymous
     * property of a context. The specific methods on the context builder, or
     * attributes builder, should be used.
     *
     * @param name The name of the attribute.
     * @param value The value for the attribute.
     * @param private_attribute If the attribute should be considered private:
     * that is, the value will not be sent to LaunchDarkly in analytics events.
     * @return A reference to the current builder.
     */
    AttributesBuilder& set(std::string name, launchdarkly::Value value);

    /**
     * Add or update a private attribute in the context.
     *
     * This method cannot be used to set the key, kind, name, or anonymous
     * property of a context. The specific methods on the context builder, or
     * attributes builder, should be used.
     *
     * Once you have set an attribute private it will remain in the private
     * list even if you call `set` afterward. This method is just a convenience
     * which also adds the attribute to the `private_attributes`.
     *
     * @param name The name of the attribute.
     * @param value The value for the attribute.
     * @param private_attribute If the attribute should be considered private:
     * that is, the value will not be sent to LaunchDarkly in analytics events.
     * @return A reference to the current builder.
     */
    AttributesBuilder& set_private(std::string name, launchdarkly::Value value);

    /**
     * Designate a context attribute, or properties within them, as private:
     * that is, their values will not be sent to LaunchDarkly in analytics
     * events.
     *
     * Each parameter can be a simple attribute name, such as "email". Or, if
     * the first character is a slash, the parameter is interpreted as a
     * slash-delimited path to a property within a JSON object, where the first
     * path component is a Context attribute name and each following component
     * is a nested property name: for example, suppose the attribute "address"
     * had the following JSON object value:
     *
     * ```
     * {"street": {"line1": "abc", "line2": "def"}}
     * ```
     *
     * Using ["/address/street/line1"] in this case would cause the "line1"
     * property to be marked as private. This syntax deliberately resembles JSON
     * Pointer, but other JSON Pointer features such as array indexing are not
     * supported for Private.
     *
     * This action only affects analytics events that involve this particular
     * Context. To mark some (or all) Context attributes as private for all
     * contexts, use the overall configuration for the SDK. See [TODO] and
     * [TODO].
     *
     * The attributes "kind" and "key", and the "_meta" attributes cannot be
     * made private.
     *
     * In this example, firstName is marked as private, but lastName is not:
     *
     * ```
     * const context = {
     *   kind: 'org',
     *   key: 'my-key',
     *   firstName: 'Pierre',
     *   lastName: 'Menard',
     *   _meta: {
     *     privateAttributes: ['firstName'],
     *   }
     * };
     * ```
     *
     * This is a metadata property, rather than an attribute that can be
     * addressed in evaluations: that is, a rule clause that references the
     * attribute name "privateAttributes", will not use this value, but would
     * use a "privateAttributes" attribute set on the context.
     * @param ref The reference to set private.
     * @return A reference to the current builder.
     */
    AttributesBuilder& add_private_attribute(AttributeReference ref);

    /**
     * Add items from an iterable collection. One that provides a begin/end
     * iterator and iterates over AttributeReferences or a convertible type.
     * @tparam IterType The type of iterable.
     * @param attributes The attributes to add as private.
     * @return A reference to the current builder.
     */
    template <typename IterType>
    AttributesBuilder& add_private_attributes(IterType attributes) {
        for (auto iter : attributes) {
            private_attributes_.insert(iter);
        }
        return *this;
    }

    AttributesBuilder kind(std::string kind, std::string key) {
        builder_.internal_add_kind(kind_, build_attributes());
        return builder_.kind(kind, key);
    }

    /**
     * Build the context. This method should not be called more than once.
     * It moves the builder content into the built context.
     *
     * @return The built context.
     */
    [[nodiscard]] BuildType build() {
        builder_.internal_add_kind(kind_, build_attributes());
        return builder_.build();
    }

   private:
    BuilderReturn& builder_;

    Attributes build_attributes();

    AttributesBuilder& set(std::string name,
                           launchdarkly::Value value,
                           bool private_attribute);

    std::string kind_;
    std::string key_;
    std::string name_;
    bool anonymous_ = false;

    std::map<std::string, launchdarkly::Value> values_;
    AttributeReference::SetType private_attributes_;
};
}  // namespace launchdarkly
