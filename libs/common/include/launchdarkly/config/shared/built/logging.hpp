#pragma once

#include <launchdarkly/logging/log_backend.hpp>

#include <optional>

namespace launchdarkly::config::shared::built {

/**
 * Logging configuration.
 */
struct Logging {
    /*
     * True to disable logging.
     */
    bool disable_logging;

    /**
     * Set to use a custom back-end.
     *
     * If set then name and level will not be used.
     */
    std::shared_ptr<ILogBackend> backend;

    /**
     * When logging is enabled, and a custom backend is not
     * in use, this will be the tag used for the logger.
     */
    std::string tag;

    /**
     * When logging is enabled, and a custom backend is not
     * in use, this will be the minimum log level.
     */
    LogLevel level;
};

}  // namespace launchdarkly::config::shared::built
