#pragma once

#include <string>
#include "attributes_builder.hpp"
#include "context.hpp"

namespace launchdarkly {

/**
 * Class for building LaunchDarkly contexts.
 *
 * You cannot build a context until you have added at least one kind.
 *
 * Building a context with a single kind.
 * @code
 * auto context = ContextBuilder()
 *      .kind("user", "bobby-bobberson")
 *      .name("Bob")
 *      .anonymous(false)
 *      // Set a custom attribute.
 *      .set("likesCats", true)
 *      // Set a private custom attribute.
 *      .set("email", "email@email.email", true)
 *      .build();
 * @endcode
 *
 * Building a context with multiple kinds.
 * @code
 * auto context = ContextBuilder()
 *      .kind("user", "bobby-bobberson")
 *      .name("Bob")
 *      .anonymous(false)
 *      // Set a custom attribute.
 *      .set("likesCats", true)
 *      // Set a private custom attribute.
 *      .set("email", "email@email.email", true)
 *      // Add another kind to the context.
 *      .kind("org", "org-key")
 *      .anonymous(true)
 *      .set("goal", "money")
 *      .build();
 * @endcode
 */
class ContextBuilder {
    friend AttributesBuilder<ContextBuilder, Context>;

   public:
    /**
     * Start adding a kind to the context.
     * @param kind The kind being added.
     * @param key The key for the kind.
     * @return A builder which allows adding attributes for the kind.
     */
    AttributesBuilder<ContextBuilder, Context> kind(std::string kind,
                                                    std::string key);

   private:
    Context build();

    void internal_add_kind(std::string kind, Attributes attrs);

    std::map<std::string, Attributes> kinds_;
    bool valid_ = true;
    std::string errors_;
};

}  // namespace launchdarkly
