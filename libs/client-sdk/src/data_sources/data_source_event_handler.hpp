#pragma once

#include <boost/asio/any_io_executor.hpp>

#include "data_source_status_manager.hpp"
#include "data_source_update_sink.hpp"

#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/data_sources/data_source.hpp>
#include <launchdarkly/logging/logger.hpp>

namespace launchdarkly::client_side::data_sources {

/**
 * This class handles LaunchDarkly events, parses them, and then uses
 * a IDataSourceUpdateSink to process the parsed events.
 *
 * This can be used for streaming or for polling. For polling only "put" events
 * will be used.
 */
class DataSourceEventHandler {
   public:
    /**
     * Status indicating if the message was processed, or if there
     * was an issue encountered.
     */
    enum class MessageStatus {
        kMessageHandled,
        kInvalidMessage,
        kUnhandledVerb
    };

    /**
     * Represents patch JSON from the LaunchDarkly service.
     */
    struct PatchData {
        std::string key;
        EvaluationResult flag;
    };

    /**
     * Represents delete JSON from the LaunchDarkly service.
     */
    struct DeleteData {
        std::string key;
        uint64_t version;
    };

    DataSourceEventHandler(Context const& context,
                           IDataSourceUpdateSink& handler,
                           Logger const& logger,
                           DataSourceStatusManager& status_manager);

    /**
     * Handles an event from the LaunchDarkly service.
     * @param type The type of the event. "put"/"patch"/"delete".
     * @param data The content of the evnet.
     * @return A status indicating if the message could be handled.
     */
    MessageStatus HandleMessage(std::string const& type,
                                std::string const& data);

   private:
    IDataSourceUpdateSink& handler_;
    Logger const& logger_;
    DataSourceStatusManager& status_manager_;
    Context context_;
};
}  // namespace launchdarkly::client_side::data_sources
