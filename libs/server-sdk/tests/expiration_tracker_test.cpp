#include <gtest/gtest.h>

#include <data_components/expiration_tracker/expiration_tracker.hpp>

using namespace launchdarkly::server_side::data_components;

ExpirationTracker::TimePoint Second(uint64_t second) {
    return std::chrono::steady_clock::time_point{std::chrono::seconds{second}};
}

TEST(ExpirationTrackerTest, CanTrackUnscopedItem) {
    ExpirationTracker tracker;
    tracker.Add("Potato", Second(10));
    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State("Potato", Second(0)));

    EXPECT_EQ(ExpirationTracker::TrackState::kStale,
              tracker.State("Potato", Second(11)));
}

TEST(ExpirationTrackerTest, CanGetStateOfUntrackedUnscopedItem) {
    ExpirationTracker tracker;
    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State("Potato", Second(0)));
}

TEST(ExpirationTrackerTest, CanTrackScopedItem) {
    ExpirationTracker tracker;
    tracker.Add(DataKind::kFlag, "Potato", Second(10));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kFlag, "Potato", Second(0)));

    EXPECT_EQ(ExpirationTracker::TrackState::kStale,
              tracker.State(DataKind::kFlag, "Potato", Second(11)));

    // Is not considered unscoped.
    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State("Potato", Second(11)));

    // The wrong scope is not tracked.
    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State(DataKind::kSegment, "Potato", Second(11)));
}

TEST(ExpirationTrackerTest, CanTrackSameKeyInMultipleScopes) {
    ExpirationTracker tracker;
    tracker.Add("Potato", Second(0));
    tracker.Add(DataKind::kFlag, "Potato", Second(10));
    tracker.Add(DataKind::kSegment, "Potato", Second(20));

    EXPECT_EQ(ExpirationTracker::TrackState::kStale,
              tracker.State("Potato", Second(9)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kFlag, "Potato", Second(9)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kSegment, "Potato", Second(10)));

    EXPECT_EQ(ExpirationTracker::TrackState::kStale,
              tracker.State(DataKind::kFlag, "Potato", Second(11)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kSegment, "Potato", Second(11)));
}

TEST(ExpirationTrackerTest, CanClear) {
    ExpirationTracker tracker;
    tracker.Add("Potato", Second(0));
    tracker.Add(DataKind::kFlag, "Potato", Second(10));
    tracker.Add(DataKind::kSegment, "Potato", Second(20));

    tracker.Clear();

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State("Potato", Second(0)));

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State(DataKind::kFlag, "Potato", Second(0)));

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State(DataKind::kSegment, "Potato", Second(0)));
}

TEST(ExpirationTrackerTest, CanPrune) {
    ExpirationTracker tracker;
    tracker.Add("freshUnscoped", Second(100));
    tracker.Add(DataKind::kFlag, "freshFlag", Second(100));
    tracker.Add(DataKind::kSegment, "freshSegment", Second(100));

    tracker.Add("staleUnscoped", Second(50));
    tracker.Add(DataKind::kFlag, "staleFlag", Second(50));
    tracker.Add(DataKind::kSegment, "staleSegment", Second(50));

    auto pruned = tracker.Prune(Second(80));
    EXPECT_EQ(3, pruned.size());
    std::vector<std::pair<std::optional<DataKind>, std::string>>
        expected_pruned{{std::nullopt, "staleUnscoped"},
                        {DataKind::kFlag, "staleFlag"},
                        {DataKind::kSegment, "staleSegment"}};
    EXPECT_EQ(expected_pruned, pruned);

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State("staleUnscoped", Second(80)));

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State(DataKind::kFlag, "staleFlag", Second(80)));

    EXPECT_EQ(ExpirationTracker::TrackState::kNotTracked,
              tracker.State(DataKind::kSegment, "staleSegment", Second(80)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State("freshUnscoped", Second(80)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kFlag, "freshFlag", Second(80)));

    EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
              tracker.State(DataKind::kSegment, "freshSegment", Second(80)));
}

TEST(ExpirationTrackerTest, CanUpdateExistingExpiry) {
    ExpirationTracker tracker;

    for (std::size_t seconds = 1; seconds < 10; seconds++) {
        auto const now = Second(seconds - 1);
        auto const expiry = Second(seconds);

        tracker.Add("Potato", expiry);
        EXPECT_EQ(ExpirationTracker::TrackState::kFresh,
                  tracker.State("Potato", now));
        ASSERT_TRUE(tracker.Prune(now).empty());
    }
}
