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
 */
class FDv2CacheInitializer final : public IFDv2Initializer {
   public:
    /**
     * @param cache Local cache to read. Non-owning. Must outlive this object.
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
 * Thread-safe: Build() may be called from any thread.
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
