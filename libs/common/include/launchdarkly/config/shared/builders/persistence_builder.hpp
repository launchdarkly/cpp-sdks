#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

#include <launchdarkly/config/shared/built/persistence.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/config/shared/sdks.hpp>
#include <launchdarkly/persistence/persistence.hpp>

namespace launchdarkly::config::shared::builders {
template <typename SDK>
class PersistenceBuilder;

template <>
class PersistenceBuilder<ClientSDK> {
   public:
    class None {};

    class Custom {
       public:
        /**
         * Set the backend to use for logging. The provided back-end should
         * be thread-safe.
         * @param backend The implementation of the backend.
         * @return A reference to this builder.
         */
        Custom& Implementation(std::shared_ptr<IPersistence> implementation);

       private:
        std::shared_ptr<IPersistence> implementation_;
        friend class PersistenceBuilder;
    };

    using PersistenceType = std::variant<None, Custom>;

    PersistenceBuilder();
    PersistenceBuilder(Custom custom);
    PersistenceBuilder(None none);

    /**
     * Set the implementation of persistence.
     * @param persistence Share
     * @return
     */
    PersistenceBuilder& Persistence(PersistenceType persistence);

    /**
     * Set the maximum number of contexts to retain cached flag data for.
     *
     * Has no effect if persistence is disabled.
     *
     * @param count The number to retain cached flag data for.
     * @return A reference to this builder.
     */
    PersistenceBuilder& MaxContexts(std::size_t count);

    [[nodiscard]] built::Persistence<ClientSDK> Build() const;

   private:
    PersistenceType type_;
    std::size_t max_contexts_;
};

template <>
class PersistenceBuilder<ServerSDK> {
   public:
    [[nodiscard]] built::Persistence<ServerSDK> Build() const {
        return built::Persistence<ServerSDK>();
    }
};

}  // namespace launchdarkly::config::shared::builders
