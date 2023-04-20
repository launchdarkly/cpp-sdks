#pragma once

#include <boost/asio/any_io_executor.hpp>

#include "config/detail/built/service_endpoints.hpp"
#include "context.hpp"
#include "data/evaluation_result.hpp"
#include "launchdarkly/client_side/data_source.hpp"
#include "launchdarkly/client_side/data_source_update_sink.hpp"
#include "launchdarkly/sse/client.hpp"
#include "logger.hpp"

namespace launchdarkly::client_side::data_sources::detail {

/**
 * This class handles events source events, parses them, and then uses
 * a IDataSourceUpdateSink to process the parsed events.
 */
class StreamingDataHandler {
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

    StreamingDataHandler(std::shared_ptr<IDataSourceUpdateSink> handler,
                         Logger const& logger);

    /**
     * Handle an SSE event.
     * @param event The event to handle.
     * @return A status indicating if the message could be handled.
     */
    MessageStatus handle_message(launchdarkly::sse::Event const& event);

   private:
    std::shared_ptr<IDataSourceUpdateSink> handler_;
    Logger const& logger_;
};
}  // namespace launchdarkly::client_side::data_sources::detail
