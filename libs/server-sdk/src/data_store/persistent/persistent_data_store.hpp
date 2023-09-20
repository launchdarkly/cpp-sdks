#pragma once

#include "../../data_sources/data_source_interface.hpp"
#include "../../data_sources/data_source_update_sink.hpp"
#include "expiration_tracker.hpp"

#include <launchdarkly/detail/unreachable.hpp>
#include <launchdarkly/persistence/persistent_store_core.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace launchdarkly::server_side::data_store::persistent {

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

class PersistentStore : public data_sources::IDataSource,
                        public data_sources::IDataSourceUpdateSink {
   public:
    PersistentStore(
        std::shared_ptr<persistence::IPersistentStoreCore> core,
        std::chrono::seconds cache_refresh_time,
        std::optional<std::chrono::seconds> eviction_interval,
        std::function<std::chrono::time_point<std::chrono::steady_clock>()>
            time = []() { return std::chrono::steady_clock::now(); });

    std::shared_ptr<FlagDescriptor> GetFlag(
        std::string const& key) const override;
    std::shared_ptr<SegmentDescriptor> GetSegment(
        std::string const& key) const override;

    std::unordered_map<std::string, std::shared_ptr<FlagDescriptor>> AllFlags()
        const override;
    std::unordered_map<std::string, std::shared_ptr<SegmentDescriptor>>
    AllSegments() const override;

    bool Initialized() const override;
    std::string const& Description() const override;

    void Init(launchdarkly::data_model::SDKDataSet dataSet) override;
    void Upsert(std::string const& key, FlagDescriptor flag) override;
    void Upsert(std::string const& key, SegmentDescriptor segment) override;

    PersistentStore() = default;
    ~PersistentStore() override = default;

    PersistentStore(PersistentStore const& item) = delete;
    PersistentStore(PersistentStore&& item) = delete;
    PersistentStore& operator=(PersistentStore const&) = delete;
    PersistentStore& operator=(PersistentStore&&) = delete;

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
            default:
                launchdarkly::detail::unreachable();
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
};

}  // namespace launchdarkly::server_side::data_store::persistent
