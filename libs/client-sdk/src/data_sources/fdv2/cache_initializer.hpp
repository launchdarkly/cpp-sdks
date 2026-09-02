#pragma once

#include "ifdv2_initializer.hpp"
#include "ifdv2_initializer_factory.hpp"

#include "../../flag_manager/flag_persistence.hpp"

#include <launchdarkly/async/promise.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <string>

namespace launchdarkly::client_side::data_sources {

/**
 * Loads flag data the SDK persisted for this context on a previous run, so
 * that evaluation can begin before the network answers.
 *
 * The cache is read on the calling thread. The read is fast enough that
 * dispatching it to the executor would cost more than it saves.
 *
 * Cached data never carries a selector. A selector names a state the service
 * can compute changes against, and the SDK does not verify that persisted
 * data is intact. Asking for a delta against data that may not be what the
 * service thinks it is would corrupt the store silently. Initialization
 * therefore continues past the cache to a network source, which supplies both
 * data and a selector.
 *
 * Run() reads the cache on the calling thread and returns a future that is
 * already resolved. Close() may be called from any thread and has nothing to
 * cancel.
 */
class FDv2CacheInitializer final : public IFDv2Initializer {
   public:
    /**
     * @param cache The local cache to read. Non-owning. Must outlive this
     * object.
     * @param context The evaluation context to load data for.
     * @param logger Receives diagnostic logging.
     */
    FDv2CacheInitializer(flag_manager::FlagPersistence* cache,
                         Context context,
                         Logger const& logger);

    async::Future<FDv2SourceResult> Run() override;

    void Close() override;

    [[nodiscard]] std::string const& Identity() const override;

   private:
    flag_manager::FlagPersistence* const cache_;
    Context const context_;
    Logger logger_;
};

/**
 * Builds fresh FDv2CacheInitializer instances on demand.
 *
 * Thread-safe: Build() may be called from any thread. The cache pointer and
 * context it hands to each initializer are fixed at construction.
 */
class FDv2CacheInitializerFactory final : public IFDv2InitializerFactory {
   public:
    FDv2CacheInitializerFactory(flag_manager::FlagPersistence* cache,
                                Context context,
                                Logger const& logger);

    std::unique_ptr<IFDv2Initializer> Build() override;

    [[nodiscard]] bool IsFromCache() const override { return true; }

   private:
    flag_manager::FlagPersistence* const cache_;
    Context const context_;
    Logger logger_;
};

}  // namespace launchdarkly::client_side::data_sources
