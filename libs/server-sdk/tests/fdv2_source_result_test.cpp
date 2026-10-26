#include <gtest/gtest.h>

#include <data_interfaces/source/fdv2_source_result.hpp>

#include <chrono>
#include <set>

using namespace launchdarkly::server_side::data_interfaces;
using namespace std::chrono_literals;

TEST(FDv1FallbackDirectiveTests, UsesAServiceSuppliedTtlAsGiven) {
    // The service jitters the TTLs it supplies, so the SDK must not.
    for (auto ttl : {1s, 60s, 3599s, 3600s}) {
        EXPECT_EQ(ttl, FDv1FallbackDirective::FromServiceTtl(ttl).ttl);
    }
}

TEST(FDv1FallbackDirectiveTests, JittersTheDefaultWhenNoTtlIsSupplied) {
    auto const directive = FDv1FallbackDirective::DefaultTtl();

    // The jittered 1-hour default lands in [30min, 1h].
    EXPECT_GE(directive.ttl, 30min);
    EXPECT_LE(directive.ttl, 1h);
}

TEST(FDv1FallbackDirectiveTests, FallsBackToTheDefaultForOutOfRangeTtls) {
    for (auto ttl : {0s, -60s, 3601s, std::chrono::seconds{24h * 7}}) {
        auto const directive = FDv1FallbackDirective::FromServiceTtl(ttl);

        // The jittered 1-hour default lands in [30min, 1h].
        EXPECT_GE(directive.ttl, 30min);
        EXPECT_LE(directive.ttl, 1h);
    }
}

TEST(FDv1FallbackDirectiveTests, ParsesATtlHeaderValue) {
    EXPECT_EQ(120s, FDv1FallbackDirective::FromServiceTtl("120").ttl);
}

TEST(FDv1FallbackDirectiveTests, TreatsAMalformedTtlHeaderAsAbsent) {
    for (auto const* value : {"", "abc", "12.5", "60s", "-60", " 60", "0"}) {
        auto const directive = FDv1FallbackDirective::FromServiceTtl(value);

        // The jittered 1-hour default lands in [30min, 1h].
        EXPECT_GE(directive.ttl, 30min);
        EXPECT_LE(directive.ttl, 1h);
    }
}

TEST(FDv1FallbackDirectiveTests, DefaultTtlJitterVaries) {
    std::set<std::chrono::seconds> observed;
    for (int i = 0; i < 50; i++) {
        observed.insert(FDv1FallbackDirective::DefaultTtl().ttl);
    }

    EXPECT_GT(observed.size(), 1u);
}
