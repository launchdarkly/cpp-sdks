#pragma once

#include <cstdint>

#include <boost/asio/any_io_executor.hpp>

#include "../data_store/data_kind.hpp"
#include "data_source_status_manager.hpp"
#include "data_source_update_sink.hpp"

#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/context.hpp>
#include <launchdarkly/data/evaluation_result.hpp>
#include <launchdarkly/data_sources/data_source.hpp>
#include <launchdarkly/logging/logger.hpp>

char const flags_path[] = "/flags/";
char const segments_path[] = "/segments/";

namespace launchdarkly::server_side::data_sources {

template <data_store::DataKind kind, char const* path>
class StreamingDataKind {
   public:
    static data_store::DataKind Kind() { return kind; }
    static bool IsKind(std::string const& patch_path) {
        return patch_path.rfind(path) == 0;
    }
    static std::string Key(std::string const& patch_path) {
        return patch_path.substr(std::char_traits<char>::length(path));
    }
};

struct StreamingDataKinds {
    using Flag = StreamingDataKind<data_store::DataKind::kFlag, flags_path>;
    using Segment =
        StreamingDataKind<data_store::DataKind::kSegment, segments_path>;

    static std::optional<data_store::DataKind> Kind(std::string const& path) {
        if (Flag::IsKind(path)) {
            return data_store::DataKind::kFlag;
        } else if (Segment::IsKind(path)) {
            return data_store::DataKind::kSegment;
        }
        return std::nullopt;
    }

    static std::optional<std::string> Key(std::string const& path) {
        if (Flag::IsKind(path)) {
            return Flag::Key(path);
        } else if (Segment::IsKind(path)) {
            return Segment::Key(path);
        }
        return std::nullopt;
    }
};

/**
 * This class handles LaunchDarkly events, parses them, and then uses
 * a IDataSourceUpdateSink to process the parsed events.
 *
 * This is only used for streaming. For server polling the shape of the poll
 * response is different than the put, so there is limited utility in
 * sharing this handler.
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

    struct Put {
        data_model::SDKDataSet data;
    };

    struct Patch {
        std::string key;
        std::variant<data_store::FlagDescriptor, data_store::SegmentDescriptor>
            data;
    };

    struct Delete {
        std::string key;
        data_store::DataKind kind;
        uint64_t version;
    };

    DataSourceEventHandler(IDataSourceUpdateSink& handler,
                           Logger const& logger,
                           DataSourceStatusManager& status_manager);

    /**
     * Handles an event from the LaunchDarkly service.
     * @param type The type of the event. "put"/"patch"/"delete".
     * @param data The content of the event.
     * @return A status indicating if the message could be handled.
     */
    MessageStatus HandleMessage(std::string const& type,
                                std::string const& data);

   private:
    IDataSourceUpdateSink& handler_;
    Logger const& logger_;
    DataSourceStatusManager& status_manager_;
};
}  // namespace launchdarkly::server_side::data_sources
