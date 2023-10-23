#include <gtest/gtest.h>

#include "data_store/dependency_tracker.hpp"
#include "data_store/descriptors.hpp"

using launchdarkly::server_side::data_store::DataKind;
using launchdarkly::server_side::data_store::DependencyMap;
using launchdarkly::server_side::data_store::DependencySet;
using launchdarkly::server_side::data_store::DependencyTracker;
using launchdarkly::server_side::data_store::FlagDescriptor;
using launchdarkly::server_side::data_store::SegmentDescriptor;

using launchdarkly::AttributeReference;
using launchdarkly::Value;
using launchdarkly::data_model::Clause;
using launchdarkly::data_model::ContextKind;
using launchdarkly::data_model::Flag;
using launchdarkly::data_model::ItemDescriptor;
using launchdarkly::data_model::Segment;

TEST(ScopedSetTest, CanAddItem) {
    DependencySet set;
    set.Set(DataKind::kFlag, "flagA");
}

TEST(ScopedSetTest, CanCheckIfContains) {
    DependencySet set;
    set.Set(DataKind::kFlag, "flagA");

    EXPECT_TRUE(set.Contains(DataKind::kFlag, "flagA"));
    EXPECT_FALSE(set.Contains(DataKind::kFlag, "flagB"));
    EXPECT_FALSE(set.Contains(DataKind::kSegment, "flagA"));
}

TEST(ScopedSetTest, CanRemoveItem) {
    DependencySet set;
    set.Set(DataKind::kFlag, "flagA");
    set.Remove(DataKind::kFlag, "flagA");
    EXPECT_FALSE(set.Contains(DataKind::kFlag, "flagA"));
}

TEST(ScopedSetTest, CanIterate) {
    DependencySet set;
    set.Set(DataKind::kFlag, "flagA");
    set.Set(DataKind::kFlag, "flagB");
    set.Set(DataKind::kSegment, "segmentA");
    set.Set(DataKind::kSegment, "segmentB");

    auto count = 0;
    auto expectations =
        std::vector<std::string>{"flagA", "flagB", "segmentA", "segmentB"};

    for (auto& ns : set) {
        if (count == 0) {
            EXPECT_EQ(DataKind::kFlag, ns.Kind());
        } else {
            EXPECT_EQ(DataKind::kSegment, ns.Kind());
        }
        for (auto val : ns.Data()) {
            EXPECT_EQ(expectations[count], val);
            count++;
        }
    }
    EXPECT_EQ(4, count);
}

TEST(ScopedMapTest, CanAddItem) {
    DependencyMap map;
    DependencySet deps;
    deps.Set(DataKind::kSegment, "segmentA");

    map.Set(DataKind::kFlag, "flagA", deps);
}

TEST(ScopedMapTest, CanGetItem) {
    DependencyMap map;
    DependencySet deps;
    deps.Set(DataKind::kSegment, "segmentA");

    map.Set(DataKind::kFlag, "flagA", deps);

    EXPECT_TRUE(map.Get(DataKind::kFlag, "flagA")
                    ->Contains(DataKind::kSegment, "segmentA"));
}

TEST(ScopedMapTest, CanIterate) {
    DependencyMap map;

    DependencySet dep_flags;
    dep_flags.Set(DataKind::kSegment, "segmentA");
    dep_flags.Set(DataKind::kFlag, "flagB");

    DependencySet depSegments;
    depSegments.Set(DataKind::kSegment, "segmentB");

    map.Set(DataKind::kFlag, "flagA", dep_flags);
    map.Set(DataKind::kSegment, "segmentA", depSegments);

    auto expectationKeys =
        std::set<std::string>{"segmentA", "flagB", "segmentB"};
    auto expectationKinds = std::vector<DataKind>{
        DataKind::kFlag, DataKind::kSegment, DataKind::kSegment};

    auto count = 0;
    for (auto& ns : map) {
        if (count == 0) {
            EXPECT_EQ(DataKind::kFlag, ns.Kind());
        } else {
            EXPECT_EQ(DataKind::kSegment, ns.Kind());
        }
        for (auto const& depSet : ns.Data()) {
            for (auto const& deps : depSet.second) {
                for (auto& dep : deps.Data()) {
                    EXPECT_EQ(expectationKinds[count], deps.Kind());
                    EXPECT_TRUE(expectationKeys.count(dep) != 0);
                    expectationKeys.erase(dep);
                    count++;
                }
            }
        }
    }
    EXPECT_EQ(3, count);
}

TEST(ScopedMapTest, CanClear) {
    DependencyMap map;

    DependencySet dep_flags;
    dep_flags.Set(DataKind::kSegment, "segmentA");
    dep_flags.Set(DataKind::kFlag, "flagB");

    DependencySet dep_segments;
    dep_segments.Set(DataKind::kSegment, "segmentB");

    map.Set(DataKind::kFlag, "flagA", dep_flags);
    map.Set(DataKind::kSegment, "segmentA", dep_segments);
    map.Clear();

    for (auto& ns : map) {
        for ([[maybe_unused]] auto& set_set : ns.Data()) {
            GTEST_FAIL();
        }
    }
}

TEST(DependencyTrackerTest, TreatsPrerequisitesAsDependencies) {
    DependencyTracker tracker;

    Flag flag_a;
    Flag flag_b;
    Flag flag_c;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    // Unused, to make sure not everything is just included in the dependencies.
    flag_c.key = "flagC";
    flag_c.version = 1;

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    tracker.UpdateDependencies("flagA", FlagDescriptor(flag_a));
    tracker.UpdateDependencies("flagB", FlagDescriptor(flag_b));
    tracker.UpdateDependencies("flagC", FlagDescriptor(flag_c));

    DependencySet changes;
    tracker.CalculateChanges(DataKind::kFlag, "flagA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagB"));
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, UsesSegmentRulesToCalculateDependencies) {
    DependencyTracker tracker;

    Flag flag_a;
    Segment segment_a;

    Flag flag_b;
    Segment segment_b;

    flag_a.key = "flagA";
    flag_a.version = 1;

    segment_a.key = "segmentA";
    segment_a.version = 1;

    // flagB and segmentB are unused.
    flag_b.key = "flagB";
    flag_b.version = 1;

    segment_b.key = "segmentB";
    segment_b.version = 1;

    flag_a.rules.push_back(Flag::Rule{std::vector<Clause>{
        Clause{Clause::Op::kSegmentMatch, std::vector<Value>{"segmentA"}, false,
               ContextKind("user"), AttributeReference()}}});

    tracker.UpdateDependencies("flagA", FlagDescriptor(flag_a));
    tracker.UpdateDependencies("segmentA", SegmentDescriptor(segment_a));

    tracker.UpdateDependencies("flagB", FlagDescriptor(flag_b));
    tracker.UpdateDependencies("segmentB", SegmentDescriptor(segment_b));

    DependencySet changes;
    tracker.CalculateChanges(DataKind::kSegment, "segmentA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagA"));
    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, TracksSegmentDependencyOfPrerequisite) {
    DependencyTracker tracker;

    Flag flag_a;
    Flag flag_b;
    Segment segment_a;

    flag_a.key = "flagA";
    flag_a.version = 1;

    flag_b.key = "flagB";
    flag_b.version = 1;

    segment_a.key = "segmentA";
    segment_a.version = 1;

    flag_a.rules.push_back(Flag::Rule{std::vector<Clause>{
        Clause{Clause::Op::kSegmentMatch, std::vector<Value>{"segmentA"}, false,
               ContextKind(""), AttributeReference()}}});

    flag_b.prerequisites.push_back(Flag::Prerequisite{"flagA", 0});

    tracker.UpdateDependencies("flagA", FlagDescriptor(flag_a));
    tracker.UpdateDependencies("flagB", FlagDescriptor(flag_b));
    tracker.UpdateDependencies("segmentA", SegmentDescriptor(segment_a));

    DependencySet changes;
    tracker.CalculateChanges(DataKind::kSegment, "segmentA", changes);

    // The segment itself was changed.
    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentA"));
    // flagA has a rule which depends on segmentA.
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagA"));
    // flagB has a prerequisite of flagA.
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagB"));
    EXPECT_EQ(3, changes.Size());
}

TEST(DependencyTrackerTest, HandlesSegmentsDependentOnOtherSegments) {
    DependencyTracker tracker;

    Segment segment_a;
    Segment segment_b;
    Segment segment_c;

    segment_a.key = "segmentA";
    segment_a.version = 1;

    segment_b.key = "segmentB";
    segment_b.version = 1;

    segment_c.key = "segmentC";
    segment_c.version = 1;

    segment_b.rules.push_back(Segment::Rule{
        std::vector<Clause>{Clause{Clause::Op::kSegmentMatch,
                                   std::vector<Value>{"segmentA"}, false,
                                   ContextKind("user"), AttributeReference()}},
        std::nullopt, std::nullopt, ContextKind(""), AttributeReference()});

    tracker.UpdateDependencies("segmentA", SegmentDescriptor(segment_a));
    tracker.UpdateDependencies("segmentB", SegmentDescriptor(segment_b));
    tracker.UpdateDependencies("segmentC", SegmentDescriptor(segment_c));

    DependencySet changes;
    tracker.CalculateChanges(DataKind::kSegment, "segmentA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentB"));
    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, HandlesUpdateForSomethingThatDoesNotExist) {
    // This shouldn't happen, but it should also not break.
    DependencyTracker tracker;

    DependencySet changes;
    tracker.CalculateChanges(DataKind::kFlag, "potato", changes);

    EXPECT_EQ(1, changes.Size());
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "potato"));
}
