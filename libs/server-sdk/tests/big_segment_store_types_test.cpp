#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>

using launchdarkly::server_side::integrations::Membership;

TEST(MembershipTests, EmptyHasNoEntries) {
    auto const m = Membership::FromSegmentRefs({}, {});
    ASSERT_FALSE(m.CheckMembership("seg.g1").has_value());
}

TEST(MembershipTests, IncludedReturnsTrue) {
    auto const m = Membership::FromSegmentRefs({"seg1.g1", "seg2.g1"}, {});
    ASSERT_EQ(m.CheckMembership("seg1.g1"), true);
    ASSERT_EQ(m.CheckMembership("seg2.g1"), true);
}

TEST(MembershipTests, ExcludedReturnsFalse) {
    auto const m = Membership::FromSegmentRefs({}, {"seg1.g1", "seg2.g1"});
    ASSERT_EQ(m.CheckMembership("seg1.g1"), false);
    ASSERT_EQ(m.CheckMembership("seg2.g1"), false);
}

TEST(MembershipTests, UnknownRefReturnsNullopt) {
    auto const m = Membership::FromSegmentRefs({"seg1.g1"}, {"seg2.g1"});
    ASSERT_FALSE(m.CheckMembership("seg3.g1").has_value());
}

TEST(MembershipTests, InclusionWinsOverExclusion) {
    // Same ref in both lists should resolve to "included" per spec.
    auto const m = Membership::FromSegmentRefs({"seg.g1"}, {"seg.g1"});
    ASSERT_EQ(m.CheckMembership("seg.g1"), true);
}

TEST(MembershipTests, DifferentGenerationsAreDistinct) {
    auto const m = Membership::FromSegmentRefs({"seg.g2"}, {"seg.g1"});
    ASSERT_EQ(m.CheckMembership("seg.g1"), false);
    ASSERT_EQ(m.CheckMembership("seg.g2"), true);
}
