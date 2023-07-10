#include <gtest/gtest.h>

#include "data_store/dependency_tracker.hpp"

using launchdarkly::server_side::data_store::ScopedSet;
using launchdarkly::server_side::data_store::ScopedMap;
using launchdarkly::server_side::data_store::DataKind;
using launchdarkly::server_side::data_store::DependencyTracker;

using launchdarkly::data_model::Flag;
using launchdarkly::data_model::Segment;
using launchdarkly::data_model::ItemDescriptor;
using launchdarkly::data_model::Clause;
using launchdarkly::Value;
using launchdarkly::AttributeReference;

TEST(ScopedSetTest, CanAddItem) {
    ScopedSet<std::string> set;
    set.Set(DataKind::kFlag, "flagA");
}

TEST(ScopedSetTest, CanCheckIfContains) {
    ScopedSet<std::string> set;
    set.Set(DataKind::kFlag, "flagA");

    EXPECT_TRUE(set.Contains(DataKind::kFlag, "flagA"));
    EXPECT_FALSE(set.Contains(DataKind::kFlag, "flagB"));
    EXPECT_FALSE(set.Contains(DataKind::kSegment, "flagA"));
}

TEST(ScopedSetTest, CanRemoveItem) {
    ScopedSet<std::string> set;
    set.Set(DataKind::kFlag, "flagA");
    set.Remove(DataKind::kFlag, "flagA");
    EXPECT_FALSE(set.Contains(DataKind::kFlag, "flagA"));
}

TEST(ScopedSetTest, CanIterate) {
    ScopedSet<std::string> set;
    set.Set(DataKind::kFlag, "flagA");
    set.Set(DataKind::kFlag, "flagB");
    set.Set(DataKind::kSegment, "segmentA");
    set.Set(DataKind::kSegment, "segmentB");

    auto count = 0;
    auto expectations = std::vector<std::string>{"flagA", "flagB", "segmentA", "segmentB"};

    for(auto& ns: set) {
        if(count == 0) {
            EXPECT_EQ(DataKind::kFlag, ns.Kind());
        } else {
            EXPECT_EQ(DataKind::kSegment, ns.Kind());
        }
        for(auto val: ns.Data()) {
            EXPECT_EQ(expectations[count], val);
            count++;
        }
    }
    EXPECT_EQ(4, count);
}

TEST(ScopedMapTest, CanAddItem) {
    ScopedMap<ScopedSet<std::string>> map;
    ScopedSet<std::string> deps;
    deps.Set(DataKind::kSegment, "segmentA");

    map.Set(DataKind::kFlag, "flagA", deps);
}

TEST(ScopedMapTest, CanGetItem) {
    ScopedMap<ScopedSet<std::string>> map;
    ScopedSet<std::string> deps;
    deps.Set(DataKind::kSegment, "segmentA");

    map.Set(DataKind::kFlag, "flagA", deps);

    EXPECT_TRUE(map.Get(DataKind::kFlag, "flagA")->Contains(DataKind::kSegment, "segmentA"));
}

TEST(ScopedMapTest, CanIterate) {
    ScopedMap<ScopedSet<std::string>> map;

    ScopedSet<std::string> depFlags;
    depFlags.Set(DataKind::kSegment, "segmentA");
    depFlags.Set(DataKind::kFlag, "flagB");

    ScopedSet<std::string> depSegments;
    depSegments.Set(DataKind::kSegment, "segmentB");

    map.Set(DataKind::kFlag, "flagA", depFlags);
    map.Set(DataKind::kSegment, "segmentA", depSegments);

    auto expectationKeys = std::set<std::string>{"segmentA", "flagB", "segmentB"};
    auto expectationKinds = std::vector<DataKind>{DataKind::kFlag, DataKind::kSegment, DataKind::kSegment};

    auto count = 0;
    for(auto& ns: map) {
        if(count == 0) {
            EXPECT_EQ(DataKind::kFlag, ns.Kind());
        } else {
            EXPECT_EQ(DataKind::kSegment, ns.Kind());
        }
        for(auto& depSet: ns.Data()) {
            for(auto& deps: depSet.second) {
                for(auto& dep: deps.Data()) {
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
    ScopedMap<ScopedSet<std::string>> map;

    ScopedSet<std::string> depFlags;
    depFlags.Set(DataKind::kSegment, "segmentA");
    depFlags.Set(DataKind::kFlag, "flagB");

    ScopedSet<std::string> depSegments;
    depSegments.Set(DataKind::kSegment, "segmentB");

    map.Set(DataKind::kFlag, "flagA", depFlags);
    map.Set(DataKind::kSegment, "segmentA", depSegments);
    map.Clear();

    for(auto& ns: map) {
        for([[maybe_unused]] auto& _depSet: ns.Data()) {
            GTEST_FAIL();
        }
    }
}

TEST(DependencyTrackerTest, TreatsPrerequisitesAsDependencies) {
    DependencyTracker tracker;

    Flag flagA;
    Flag flagB;
    Flag flagC;

    flagA.key = "flagA";
    flagA.version = 1;

    flagB.key = "flagB";
    flagB.version = 1;

    // Unused, to make sure not everything is just included in the dependencies.
    flagC.key = "flagC";
    flagC.version = 1;

    flagB.prerequisites.push_back(Flag::Prerequisite{
        "flagA",
        0
    });

    tracker.updateDependencies("flagA", ItemDescriptor<Flag>(flagA));
    tracker.updateDependencies("flagB", ItemDescriptor<Flag>(flagB));
    tracker.updateDependencies("flagC", ItemDescriptor<Flag>(flagC));

    ScopedSet<std::string> changes;
    tracker.calculateChanges(DataKind::kFlag, "flagA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagB"));
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, UsesSegmentRulesToCalculateDependencies) {
    DependencyTracker tracker;

    Flag flagA;
    Segment segmentA;

    Flag flagB;
    Segment segmentB;

    flagA.key = "flagA";
    flagA.version = 1;

    segmentA.key = "segmentA";
    segmentA.version = 1;

    // flagB and segmentB are unused.
    flagB.key = "flagB";
    flagB.version = 1;

    segmentB.key = "segmentB";
    segmentB.version = 1;

    flagA.rules.push_back(Flag::Rule {
        std::vector<Clause>{
            Clause{
                Clause::Op::kSegmentMatch,
                std::vector<Value>{"segmentA"},
                false,
                "user",
                AttributeReference()
            }
        }
    });

    tracker.updateDependencies("flagA", ItemDescriptor<Flag>(flagA));
    tracker.updateDependencies("segmentA", ItemDescriptor<Segment>(segmentA));

    tracker.updateDependencies("flagB", ItemDescriptor<Flag>(flagB));
    tracker.updateDependencies("segmentB", ItemDescriptor<Segment>(segmentB));

    ScopedSet<std::string> changes;
    tracker.calculateChanges(DataKind::kSegment, "segmentA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "flagA"));
    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, TracksSegmentDependencyOfPrerequisite) {
    DependencyTracker tracker;

    Flag flagA;
    Flag flagB;
    Segment segmentA;

    flagA.key = "flagA";
    flagA.version = 1;

    flagB.key = "flagB";
    flagB.version = 1;

    segmentA.key = "segmentA";
    segmentA.version = 1;

    flagA.rules.push_back(Flag::Rule {
        std::vector<Clause>{
            Clause{
                Clause::Op::kSegmentMatch,
                std::vector<Value>{"segmentA"},
                false,
                "",
                AttributeReference()
            }
        }
    });

    flagB.prerequisites.push_back(Flag::Prerequisite{
        "flagA",
        0
    });

    tracker.updateDependencies("flagA", ItemDescriptor<Flag>(flagA));
    tracker.updateDependencies("flagB", ItemDescriptor<Flag>(flagB));
    tracker.updateDependencies("segmentA", ItemDescriptor<Segment>(segmentA));

    ScopedSet<std::string> changes;
    tracker.calculateChanges(DataKind::kSegment, "segmentA", changes);

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

    Segment segmentA;
    Segment segmentB;
    Segment segmentC;

    segmentA.key = "segmentA";
    segmentA.version = 1;

    segmentB.key = "segmentB";
    segmentB.version = 1;

    segmentC.key = "segmentC";
    segmentC.version = 1;

    segmentB.rules.push_back(Segment::Rule {
        std::vector<Clause>{
            Clause{
                Clause::Op::kSegmentMatch,
                std::vector<Value>{"segmentA"},
                false,
                "user",
                AttributeReference()
            }
        },
        std::nullopt,
        std::nullopt,
        "",
        AttributeReference()
    });

    tracker.updateDependencies("segmentA", ItemDescriptor<Segment>(segmentA));
    tracker.updateDependencies("segmentB", ItemDescriptor<Segment>(segmentB));
    tracker.updateDependencies("segmentC", ItemDescriptor<Segment>(segmentC));

    ScopedSet<std::string> changes;
    tracker.calculateChanges(DataKind::kSegment, "segmentA", changes);

    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentB"));
    EXPECT_TRUE(changes.Contains(DataKind::kSegment, "segmentA"));
    EXPECT_EQ(2, changes.Size());
}

TEST(DependencyTrackerTest, HandlesUpdateForSomethingThatDoesNotExist) {
    // This shouldn't happen, but it should also not break.
    DependencyTracker tracker;

    ScopedSet<std::string> changes;
    tracker.calculateChanges(DataKind::kFlag, "potato", changes);

    EXPECT_EQ(1, changes.Size());
    EXPECT_TRUE(changes.Contains(DataKind::kFlag, "potato"));
}
