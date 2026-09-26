#pragma once

#include "ifdv2_initializer_factory.hpp"
#include "ifdv2_synchronizer_factory.hpp"

#include "../../flag_manager/flag_persistence.hpp"

#include <launchdarkly/config/shared/built/fdv2_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>
#include <string>
#include <vector>

namespace launchdarkly::client_side::data_sources {

using ConnectionMode = config::shared::ConnectionMode;
using FDv2Config = config::shared::built::FDv2Config<config::shared::ClientSDK>;

/** The factories one connection mode calls for. */
struct ModeSources {
    std::vector<std::unique_ptr<IFDv2InitializerFactory>> initializers;
    std::vector<std::unique_ptr<IFDv2SynchronizerFactory>> synchronizers;
};

/**
 * Everything the sources of a mode need that the mode itself does not say:
 * where to send requests, which context to evaluate, and where the cache is.
 */
struct ModeSourceParams {
    boost::asio::any_io_executor executor;
    Logger logger;
    /** Used by any source that does not configure a URL of its own. */
    std::string polling_base_url;
    std::string streaming_base_url;
    config::shared::built::HttpProperties http_properties;
    /**
     * Where the FDv1 fallback source sends its requests. FDv2 sources use
     * the resolved base URLs above instead.
     */
    config::shared::built::ServiceEndpoints endpoints;
    Context context;
    /** Whether the application asked for evaluation reasons. */
    bool with_reasons;
    /**
     * The local cache, read by the cache initializer and for the last time
     * this context was polled. Non-owning. Must outlive the sources built
     * from these params.
     */
    flag_manager::FlagPersistence* cache;
};

/**
 * Assembles the factories the given mode calls for. Returns empty lists if
 * the configuration does not define the mode.
 *
 * A mode that configures an FDv1 fallback gets its synchronizer appended to
 * the list. The orchestrator holds that tier in reserve rather than using it
 * in rotation. It is started only while the service has directed the SDK away
 * from FDv2.
 */
ModeSources BuildModeSources(FDv2Config const& config,
                             ConnectionMode mode,
                             ModeSourceParams const& params);

}  // namespace launchdarkly::client_side::data_sources
