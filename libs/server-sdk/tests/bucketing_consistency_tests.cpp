#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include "evaluation/bucketing.hpp"
#include "evaluation/evaluator.hpp"

// Note: These tests are meant to be exact duplicates of tests
// in other SDKs. Do not change any of the values unless they
// are also changed in other SDKs. These are not traditional behavioral
// tests so much as consistency tests to guarantee that the implementation
// is identical across SDKs.

class BucketingTests : public ::testing::Test {
   public:
    const static double kBucketTolerance;
};

double const BucketingTests::kBucketTolerance = 0.0000001;

using namespace launchdarkly;
using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::evaluation;
using WeightedVariation = Flag::Rollout::WeightedVariation;

TEST_F(BucketingTests, VariationIndexForContext) {
    char const* kHashKey = "hashKey";
    char const* kSalt = "saltyA";

    WeightedVariation wv0(0, 60'000);
    WeightedVariation wv1(1, 40'000);

    Flag::VariationOrRollout rollout = Flag::Rollout({wv0, wv1});

    auto result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyA").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyA should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 0)
        << "userKeyA (bucket 0.42157587) should get variation 0";
    ASSERT_EQ(result->in_experiment, false)
        << "userKeyA should not be in experiment";

    result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyB").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyB should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 1)
        << "userKeyB (bucket 0.6708485) should get variation 1";
    ASSERT_EQ(result->in_experiment, false)
        << "userKeyB should not be in experiment";

    result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyC").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyC should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 0)
        << "userKeyC (bucket 0.10343106) should get variation 0";
    ASSERT_EQ(result->in_experiment, false)
        << "userKeyC should not be in experiment";
}
