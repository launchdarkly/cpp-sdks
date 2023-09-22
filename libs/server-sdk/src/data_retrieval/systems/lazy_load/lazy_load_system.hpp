#pragma once

#include <launchdarkly/config/shared/built/data_source_config.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/config/shared/built/service_endpoints.hpp>
#include <launchdarkly/logging/logger.hpp>

#include "../interfaces/data_push_source.hpp"
#include "../interfaces/data_system.hpp"

#include "../data_source_event_handler.hpp"
#include "../data_source_status_manager.hpp"

#include "../memory_store/memory_store.hpp"

#include <boost/asio/any_io_executor.hpp>

namespace launchdarkly::server_side::data_system {

class PullModeSource : public IDataSource, public ISynchronizer {
   public:
    PullModeSource(config::shared::built::ServiceEndpoints const& endpoints,
                   config::shared::built::DataSourceConfig<
                       config::shared::ServerSDK> const& data_source_config,
                   config::shared::built::HttpProperties http_properties,
                   boost::asio::any_io_executor ioc,
                   DataSourceStatusManager& status_manager,
                   Logger const& logger);

    PullModeSource(PullModeSource const& item) = delete;
    PullModeSource(PullModeSource&& item) = delete;
    PullModeSource& operator=(PullModeSource const&) = delete;
    PullModeSource& operator=(PullModeSource&&) = delete;

    std::string const& Identity() const override;

    ISynchronizer* GetSynchronizer() override;
    IBootstrapper* GetBootstrapper() override;

    std::shared_ptr<FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string const& key) const override;
    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const override;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const override;

    /* ISynchronizer implementation */
    void Init(std::optional<data_model::SDKDataSet> initial_data,
              IDataDestination& destination) override;
    void Start() override;
    void ShutdownAsync(std::function<void()>) override;

   private:
    void RefreshAllFlags() const;
    void RefreshAllSegments() const;
    void RefreshInitState() const;
    void RefreshFlag(std::string const& key) const;
    void RefreshSegment(std::string const& key) const;

    static persistence::SerializedItemDescriptor Serialize(FlagDescriptor flag);
    static persistence::SerializedItemDescriptor Serialize(
        SegmentDescriptor segment);

    static std::optional<FlagDescriptor> DeserializeFlag(
        persistence::SerializedItemDescriptor flag);

    static std::optional<SegmentDescriptor> DeserializeSegment(
        persistence::SerializedItemDescriptor segment);

    template <typename TResult>
    static TResult Get(ExpirationTracker::TrackState state,
                       std::function<void(void)> refresh,
                       std::function<TResult(void)> get) {
        switch (state) {
            case ExpirationTracker::TrackState::kStale:
                [[fallthrough]];
            case ExpirationTracker::TrackState::kNotTracked:
                refresh();
                [[fallthrough]];
            case ExpirationTracker::TrackState::kFresh:
                return get();
        }
    }

    mutable MemoryStore memory_store_;
    std::shared_ptr<persistence::IPersistentStoreCore> core_;
    mutable ExpirationTracker tracker_;
    std::function<std::chrono::time_point<std::chrono::steady_clock>()> time_;
    mutable std::optional<bool> initialized_;

    class SegmentKind : public persistence::IPersistentKind {
       public:
        std::string const& Namespace() const override;
        uint64_t Version(std::string const& data) const override;

        ~SegmentKind() override = default;

       private:
        static inline std::string const namespace_ = "segments";
    };

    class FlagKind : public persistence::IPersistentKind {
       public:
        std::string const& Namespace() const override;
        uint64_t Version(std::string const& data) const override;

        ~FlagKind() override = default;

       private:
        static inline std::string const namespace_ = "features";
    };

    struct Kinds {
        static FlagKind const Flag;
        static SegmentKind const Segment;
    };

    struct Keys {
        static inline std::string const kAllFlags = "allFlags";
        static inline std::string const kAllSegments = "allSegments";
        static inline std::string const kInitialized = "initialized";
    };

    MemoryStore store_;
    std::shared_ptr<ISynchronizer> synchronizer_;
    std::shared_ptr<IBootstrapper> bootstrapper_;
};
}  // namespace launchdarkly::server_side::data_system
