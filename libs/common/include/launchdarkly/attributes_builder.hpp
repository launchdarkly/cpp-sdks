#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/attributes.hpp>
#include <launchdarkly/value.hpp>

#include <map>
#include <string>

namespace launchdarkly {
class ContextBuilder;

/**
 * This is used in the implementation of the context builder for setting
 * attributes for a single context. This is not intended to be directly
 * used by an SDK consumer.
 *
 * @tparam BuilderReturn The type of builder using the AttributesBuilder.
 * @tparam BuildType The type of object being built.
 */
template <class BuilderReturn, class BuildType>
class AttributesBuilder final {
    friend class ContextBuilder;

public:
    /**
     * Create an attributes builder with the given kind and key.
     * @param builder The context builder associated with this attributes
     * builder.
     * @param kind The kind being added.
     * @param key The key for the kind.
     */
    AttributesBuilder(BuilderReturn& builder, std::string kind, std::string key)
        : builder_(builder), kind_(std::move(kind)), key_(std::move(key)) {
    }

    /**
     * Crate an attributes builder with the specified kind, and pre-populated
     * with the given attributes.
     * @param builder The context builder associated with this attributes
     * builder.
     * @param kind The kind being added.
     * @param attributes Attributes to populate the builder with.
     */
    AttributesBuilder(BuilderReturn& builder,
                      std::string kind,
                      Attributes const& attributes)
        : builder_(builder),
          kind_(std::move(kind)),
          key_(attributes.Key()),
          name_(attributes.Name()),
          anonymous_(attributes.Anonymous()),
          private_attributes_(attributes.PrivateAttributes()) {
        for (auto& pair : attributes.CustomAttributes().AsObject()) {
            values_[pair.first] = pair.second;
        }
    }

    /**
     * The attributes builder should never be copied. We depend on a stable
     * reference stored in the context builder.
     */
    AttributesBuilder(AttributesBuilder const& builder) = delete;
    AttributesBuilder& operator=(AttributesBuilder const&) = delete;
    AttributesBuilder& operator=(AttributesBuilder&&) = delete;

    // This cannot be noexcept because of:
    // https://developercommunity.visualstudio.com/t/bug-in-stdmapstdpair-implementation-with-move-only/840554
    AttributesBuilder(AttributesBuilder&& builder) = default;
    ~AttributesBuilder() = default;

    /**
     * The context's name.
     *
     * You can search for contexts on the Contexts page by name.
     *
     * @param name
     * @return A reference to the current builder.
     */
    AttributesBuilder& Name(std::string name);

    /**
     * If true, the context will _not_ appear on the Contexts page in the
     * LaunchDarkly dashboard.
     *
     * @param anonymous The value to set.
     * @return A reference to the current builder.
     */
    AttributesBuilder& Anonymous(bool anonymous);

    /**
     * Add or update an attribute in the context.
     *
     * This method cannot be used to set the key, kind, name, anonymous, or
     * _meta property of a context. The specific methods on the context builder,
     * or attributes builder, should be used.
     *
     * @param name The name of the attribute.
     * @param value The value for the attribute.
     * @return A reference to the current builder.
     */
    AttributesBuilder& Set(std::string name, Value value);

    /**
     * Add or update a private attribute in the context.
     *
     * This method cannot be used to set the key, kind, name, or anonymous
     * property of a context. The specific methods on the context builder, or
     * attributes builder, should be used.
     *
     * Once you have set an attribute private it will remain in the private
     * list even if you call `set` afterward. This method is just a convenience
     * which also adds the attribute to the `PrivateAttributes`.
     *
     * @param name The name of the attribute.
     * @param value The value for the attribute.
     * @return A reference to the current builder.
     */
    AttributesBuilder& SetPrivate(std::string name, Value value);

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
     * contexts, use the overall configuration for the SDK. See
     * launchdarkly::config::shared::builders::EventsBuilder<SDK>::AllAttributesPrivate
     * and
     * launchdarkly::config::shared::builders::EventsBuilder<SDK>::PrivateAttribute.
     *
     * The attributes "kind" and "key", and the "_meta" attributes cannot be
     * made private.
     *
     * In this example, firstName is marked as private, but lastName is not:
     *
     * ```
     * [TODO]
     * ```
     *
     * This is a metadata property, rather than an attribute that can be
     * addressed in evaluations: that is, a rule clause that references the
     * attribute name "privateAttributes", will not use this value, but would
     * use a "privateAttributes" attribute set on the context.
     * @param ref The reference to set private.
     * @return A reference to the current builder.
     */
    AttributesBuilder& AddPrivateAttribute(AttributeReference ref);

    /**
     * Add items from an iterable collection. One that provides a begin/end
     * iterator and iterates over AttributeReferences or a convertible type.
     * @tparam IterType The type of iterable.
     * @param attributes The attributes to add as private.
     * @return A reference to the current builder.
     */
    template <typename IterType>
    AttributesBuilder& AddPrivateAttributes(IterType attributes) {
        for (auto iter : attributes) {
            private_attributes_.insert(iter);
        }
        return *this;
    }

    /**
     * Start adding a kind to the context.
     *
     * If you call this function multiple times with the same kind, then
     * the same builder will be returned each time. If you previously called
     * the function with the same kind, but different key, then the key
     * will be updated.
     *
     * @param kind The kind being added.
     * @param key The key for the kind.
     * @return A builder which allows adding attributes for the kind.
     */
    AttributesBuilder& Kind(std::string kind, std::string key) {
        return builder_.Kind(kind, key);
    }

    /**
     * Start updating an existing kind.
     *
     * @param kind The kind to start updating.
     * @return A builder which allows adding attributes for the kind, or
     * nullptr if the kind doesn't already exist.
     */
    AttributesBuilder* Kind(std::string const& kind) {
        return builder_.Kind(kind);
    }

    /**
     * Build the context.
     *
     * @return The built context.
     */
    [[nodiscard]] BuildType Build() const { return builder_.Build(); }

private:
    BuilderReturn& builder_;

    /**
     * Used internally for updating the attributes key.
     * @param key The key to replace the existing key.
     * @return A reference to this builder.
     */
    void Key(std::string key) { key_ = std::move(key); }

    Attributes BuildAttributes() const;

    AttributesBuilder& Set(std::string name,
                           Value value,
                           bool private_attribute);


    std::string kind_;
    std::string key_;
    std::string name_;
    bool anonymous_ = false;

    std::map<std::string, Value> values_;
    AttributeReference::SetType private_attributes_;
};
} // namespace launchdarkly
