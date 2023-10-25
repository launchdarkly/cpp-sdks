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
    class NoneBuilder {};

    class CustomBuilder {
       public:
        /**
         * Set the backend to use for logging. The provided back-end should
         * be thread-safe.
         * @param backend The implementation of the backend.
         * @return A reference to this builder.
         */
        CustomBuilder& Implementation(
            std::shared_ptr<IPersistence> implementation);

       private:
        std::shared_ptr<IPersistence> implementation_;
        friend class PersistenceBuilder;
    };

    using PersistenceType = std::variant<NoneBuilder, CustomBuilder>;

    PersistenceBuilder();

    /**
     * Set the implementation of persistence.
     *
     * The Custom and None convenience methods can be used to directly
     * set the persistence type.
     *
     * @param persistence The builder for the type of persistence.
     * @return A reference to this builder.
     */
    PersistenceBuilder& Type(PersistenceType persistence);

    /**
     * Set the persistence to a custom implementation.
     *
     * @return A reference to this builder.
     */
    PersistenceBuilder& Custom(std::shared_ptr<IPersistence> implementation);

    /**
     * Disables persistence.
     * @return A reference to this builder.
     */
    PersistenceBuilder& None();

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
    PersistenceBuilder()
        : persistence_(Defaults<ServerSDK>::PersistenceConfig()) {}

    //    /**
    //     * Set the core persistence implementation.
    //     *
    //     * @param core The core persistence implementation.
    //     * @return A reference to this builder.
    //     */
    //    PersistenceBuilder& Core(
    //        std::shared_ptr<persistence::IPersistentStoreCore> core);
    //
    //    /**
    //     * How long something in the cache is considered fresh.
    //     *
    //     * Each item that is cached will have its age tracked. If the age of
    //     * the item exceeds the cache refresh time, then an attempt will be
    //     made
    //     * to refresh the item next time it is requested.
    //     *
    //     * When ActiveEviction is set to false then the item will remain
    //     cached
    //     * and that cached value will be used if attempts to refresh the value
    //     fail.
    //     *
    //     * If ActiveEviction is set to true, then expired items will be
    //     periodically
    //     * removed from the cache.
    //     *
    //     * @param cache_refresh_time The time, in seconds, cached data remains
    //     * fresh.
    //     * @return A reference to this builder.
    //     */
    //    PersistenceBuilder& CacheRefreshTime(
    //        std::chrono::seconds cache_refresh_time) {
    //        persistence_.cache_refresh_time = cache_refresh_time;
    //        return *this;
    //    }
    //
    //    /**
    //     * Enable/disable active eviction.
    //     *
    //     * Defaults to disabled.
    //     * @param active_eviction True to enable.
    //     * @return A reference to this builder.
    //     */
    //    PersistenceBuilder& ActiveEviction(bool active_eviction) {
    //        persistence_.active_eviction = active_eviction;
    //        return *this;
    //    }
    //
    //    /**
    //     * If active eviction is enabled, then this specifies the time between
    //     * active evictions.
    //     * @param eviction_interval The interval, in seconds, between cache
    //     flushes.
    //     * @return A reference to this builder.
    //     */
    //    PersistenceBuilder& EvictionInterval(
    //        std::chrono::seconds eviction_interval) {
    //        persistence_.eviction_interval = eviction_interval;
    //        return *this;
    //    }

    [[nodiscard]] built::Persistence<ServerSDK> Build() const {
        return persistence_;
    }

   private:
    built::Persistence<ServerSDK> persistence_;
};

}  // namespace launchdarkly::config::shared::builders
