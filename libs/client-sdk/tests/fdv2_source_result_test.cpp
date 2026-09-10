#include <gtest/gtest.h>

#include <data_sources/fdv2/fdv2_source_result.hpp>
#include <data_sources/fdv2/ifdv2_initializer_factory.hpp>
#include <data_sources/fdv2/ifdv2_synchronizer_factory.hpp>

#include <chrono>
#include <set>

using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

TEST(FDv1FallbackDirectiveTests, UsesAServiceSuppliedTtlAsGiven) {
    // The service jitters the TTLs it supplies, so the SDK must not.
    for (auto ttl : {1s, 60s, 3599s, 3600s}) {
        EXPECT_EQ(ttl, FDv1FallbackDirective::FromServiceTtl(ttl).ttl);
    }
}

TEST(FDv1FallbackDirectiveTests, JittersTheDefaultWhenNoTtlIsSupplied) {
    auto const directive = FDv1FallbackDirective::FromServiceTtl(std::nullopt);

    // The jittered 1-hour default lands in [30min, 1h].
    EXPECT_GE(directive.ttl, 30min);
    EXPECT_LE(directive.ttl, 1h);
}

TEST(FDv1FallbackDirectiveTests, FallsBackToTheDefaultForOutOfRangeTtls) {
    for (auto ttl : {0s, 3601s, std::chrono::seconds{24h * 7}}) {
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
    for (auto const* value : {"", "abc", "12.5", "60s", "-60", " 60"}) {
        auto const directive = FDv1FallbackDirective::FromServiceTtl(value);

        // The jittered 1-hour default lands in [30min, 1h].
        EXPECT_GE(directive.ttl, 30min);
        EXPECT_LE(directive.ttl, 1h);
    }
}

TEST(FDv1FallbackDirectiveTests, DefaultTtlJitterVaries) {
    std::set<std::chrono::seconds> observed;
    for (int i = 0; i < 50; i++) {
        observed.insert(
            FDv1FallbackDirective::FromServiceTtl(std::nullopt).ttl);
    }

    EXPECT_GT(observed.size(), 1u);
}

TEST(FDv2SourceFactoryTests, FactoriesAreNeitherCacheNorFDv1ByDefault) {
    class Initializers final : public IFDv2InitializerFactory {
       public:
        std::unique_ptr<IFDv2Initializer> Build() override { return nullptr; }
    };
    class Synchronizers final : public IFDv2SynchronizerFactory {
       public:
        std::unique_ptr<IFDv2Synchronizer> Build() override { return nullptr; }
    };

    EXPECT_FALSE(Initializers{}.IsFromCache());
    EXPECT_FALSE(Synchronizers{}.IsFDv1Fallback());
}
