#pragma once

#include <launchdarkly/attributes_builder.hpp>
#include <launchdarkly/context.hpp>

#include <string>

namespace launchdarkly {

/**
 * Class for building LaunchDarkly contexts.
 *
 * You cannot build a context until you have added at least one kind.
 *
 * Building a context with a single kind.
 * @code
 * auto context = ContextBuilder()
 *      .Kind("user", "bobby-bobberson")
 *      .Name("Bob")
 *      .Anonymous(false)
 *      // Set a custom attribute.
 *      .Set("likesCats", true)
 *      // Set a private custom attribute.
 *      .SetPrivate("email", "email@email.email")
 *      .Build();
 * @endcode
 *
 * Building a context with multiple Kinds.
 * @code
 * auto context = ContextBuilder()
 *      .Kind("user", "bobby-bobberson")
 *      .Name("Bob")
 *      .Anonymous(false)
 *      // Set a custom attribute.
 *      .Set("likesCats", true)
 *      // Set a private custom attribute.
 *      .SetPrivate("email", "email@email.email")
 *      // Add another kind to the context.
 *      .Kind("org", "org-key")
 *      .Anonymous(true)
 *      .Set("goal", "money")
 *      .Build();
 * @endcode
 *
 * Using the builder with loops.
 * @code
 * auto builder = ContextBuilder();
 * // The data in this sample is not realistic, but it is intended to show
 * // how to use the builder with loops.
 * for (auto const& kind : Kinds) { // Some collection we are using to make
 * Kinds.
 *     // The `kind` method returns a reference, always store it in a reference.
 *     auto& kind_builder = builder.Kind(kind, kind + "-key");
 *     for (auto const& prop : props) { // A collection of props we want to add.
 *         kind_builder.Set(prop.first, prop.second);
 *     }
 * }
 *
 * auto context = builder.Build();
 * @endcode
 */
class ContextBuilder final {
    friend AttributesBuilder<ContextBuilder, Context>;

   public:
    ContextBuilder() = default;

    /**
     * Create a new context builder from the given context. The created builder
     * will have all the kinds and attributes of the original context.
     *
     * If the original context is not valid, then this builder will
     * be created in a default state.
     *
     * @param context The context to base the builder on.
     */
    ContextBuilder(Context const& context);

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
    AttributesBuilder<ContextBuilder, Context>& Kind(std::string const& kind,
                                                     std::string key);

    /**
     * Start updating an existing kind.
     *
     * @param kind The kind to start updating.
     * @return A builder which allows adding attributes for the kind, or
     * nullptr if the kind doesn't already exist.
     */
    AttributesBuilder<ContextBuilder, Context>* Update(std::string kind);

    /**
     * Build a context. The same builder instance may be used to build multiple
     * contexts.
     *
     * You MUST add at least one kind before building a context. Not doing
     * so will result in an invalid context.
     *
     * @return The built context.
     */
    Context Build() const;

   private:
    std::map<std::string, AttributesBuilder<ContextBuilder, Context>> builders_;
};

}  // namespace launchdarkly
