#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/data_model/flag.hpp>
#include "evaluation/bucketing.hpp"
#include "evaluation/evaluator.hpp"

using namespace launchdarkly;
using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::evaluation;
using WeightedVariation = Flag::Rollout::WeightedVariation;

// Note: These tests are meant to be exact duplicates of tests
// in other SDKs. Do not change any of the values unless they
// are also changed in other SDKs. These are not traditional behavioral
// tests so much as consistency tests to guarantee that the implementation
// is identical across SDKs.

class BucketingTests : public ::testing::Test {
   public:
    const static double kBucketTolerance;
    const static std::string kHashKey;
    const static std::string kSalt;
};

double const BucketingTests::kBucketTolerance = 0.0000001;
std::string const BucketingTests::kHashKey = "hashKey";
std::string const BucketingTests::kSalt = "saltyA";

TEST_F(BucketingTests, VariationIndexForContext) {
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

TEST_F(BucketingTests, VariationIndexForContextWithCustomAttribute) {
    WeightedVariation wv0(0, 60'000);
    WeightedVariation wv1(1, 40'000);

    Flag::Rollout rollout({wv0, wv1});
    rollout.bucketBy = "intAttr";

    auto result = Variation(rollout, kHashKey,
                            ContextBuilder()
                                .Kind("user", "userKeyA")
                                .Set("intAttr", 33'333)
                                .Build(),
                            kSalt);

    ASSERT_TRUE(result)
        << "userKeyA should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 0)
        << "userKeyA (bucket 0.54771423) should get variation 0";

    ASSERT_EQ(result->in_experiment, false)
        << "userKeyA should not be in experiment";

    result = Variation(rollout, kHashKey,
                       ContextBuilder()
                           .Kind("user", "userKeyA")
                           .Set("intAttr", 99'999)
                           .Build(),
                       kSalt);

    ASSERT_TRUE(result)
        << "userKeyA should be assigned a bucket result, but got: "
        << result.error();
    ASSERT_EQ(result->variation_index, 1)
        << "userKeyA (bucket 0.7309658) should get variation 1";

    ASSERT_EQ(result->in_experiment, false)
        << "userKeyA should not be in experiment";
}

TEST_F(BucketingTests, VariationIndexForContextInExperiment) {
    auto wv0 = WeightedVariation(0, 10'000);
    auto wv1 = WeightedVariation(1, 20'000);
    auto wv0_untracked = WeightedVariation::Untracked(0, 70'000);

    Flag::Rollout rollout({wv0, wv1, wv0_untracked});
    rollout.kind = Flag::Rollout::Kind::kExperiment;
    rollout.seed = 61;

    auto result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyA").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyA should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 0)
        << "userKeyA (bucket 0.09801207) should get variation 0";

    ASSERT_TRUE(result->in_experiment) << "userKeyA should be in experiment";

    result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyB").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyB should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 1)
        << "userKeyB (bucket 0.14483777)  should get variation 1";

    ASSERT_TRUE(result->in_experiment) << "userKeyB should be in experiment";

    result =
        Variation(rollout, kHashKey,
                  ContextBuilder().Kind("user", "userKeyC").Build(), kSalt);

    ASSERT_TRUE(result)
        << "userKeyC should be assigned a bucket result, but got: "
        << result.error();

    ASSERT_EQ(result->variation_index, 0)
        << "userKeyC (bucket 0.9242641)   should get variation 1";

    ASSERT_FALSE(result->in_experiment)
        << "userKeyC should not be in experiment";
}

struct ExperimentBucketTest {
    std::optional<std::int64_t> seed;
    std::string key;
    Flag::Variation expectedVariationIndex;
};

class ExperimentBucketingTests
    : public BucketingTests,
      public ::testing::WithParamInterface<ExperimentBucketTest> {};

TEST_P(ExperimentBucketingTests, VariationIndexForExperiment) {
    auto const& params = GetParam();

    WeightedVariation wv0(0, 10'000);
    WeightedVariation wv1(1, 20'000);
    WeightedVariation wv2(2, 70'000);

    Flag::Rollout rollout({wv0, wv1, wv2});
    rollout.bucketBy = "numberAttr";
    rollout.kind = Flag::Rollout::Kind::kExperiment;
    rollout.seed = params.seed;

    auto result = Variation(rollout, kHashKey,
                            ContextBuilder()
                                .Kind("user", params.key)
                                .Set("numberAttr", 0.6708485)
                                .Build(),
                            kSalt);

    ASSERT_TRUE(result);

    ASSERT_EQ(result->variation_index, params.expectedVariationIndex)
        << params.key << " with seed "
        << (params.seed ? std::to_string(*params.seed) : "(none)")
        << " should get variation " << params.expectedVariationIndex;
}

INSTANTIATE_TEST_SUITE_P(
    VariationsForSeedsAndKeys,
    ExperimentBucketingTests,
    ::testing::ValuesIn({
        ExperimentBucketTest{std::nullopt, "userKeyA", 2},  // 0.42157587,
        ExperimentBucketTest{std::nullopt, "userKeyB", 2},  // 0.6708485,
        ExperimentBucketTest{std::nullopt, "userKeyC", 1},  // 0.10343106,
        ExperimentBucketTest{61, "userKeyA", 0},            // 0.09801207,
        ExperimentBucketTest{61, "userKeyB", 1},            // 0.14483777,
        ExperimentBucketTest{61, "userKeyC", 2}             // 0.9242641,
    }));

TEST_F(BucketingTests, BucketContextByKey) {
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context = ContextBuilder().Kind("user", "userKeyA").Build();
    auto result = Bucket(context, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyA should be bucketed but got "
                        << result.error();

    ASSERT_NEAR(result->first, 0.42157587, kBucketTolerance);

    context = ContextBuilder().Kind("user", "userKeyB").Build();
    result = Bucket(context, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyB should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.6708485, kBucketTolerance);

    context = ContextBuilder().Kind("user", "userKeyC").Build();
    result = Bucket(context, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyC should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.10343106, kBucketTolerance);
}

TEST_F(BucketingTests, BucketContextByKeyWithSeed) {
    auto const kPrefix = BucketPrefix::Seed(61);

    auto contextA = ContextBuilder().Kind("user", "userKeyA").Build();
    auto result = Bucket(contextA, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyA should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.09801207, kBucketTolerance);

    auto contextB = ContextBuilder().Kind("user", "userKeyB").Build();
    result = Bucket(contextB, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyB should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.14483777, kBucketTolerance);

    auto contextC = ContextBuilder().Kind("user", "userKeyC").Build();
    result = Bucket(contextC, "key", kPrefix, false, "user");
    ASSERT_TRUE(result) << "userKeyC should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.9242641, kBucketTolerance);

    // changing seed produces a different bucket value
    result = Bucket(contextA, "key", BucketPrefix::Seed(60), false, "user");
    ASSERT_TRUE(result)
        << "userKeyA with seed of 60 should be bucketed but got "
        << result.error();

    ASSERT_NEAR(result->first, 0.7008816, kBucketTolerance);
}

TEST_F(BucketingTests, BucketContextByInvalidReference) {
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);
    auto const kInvalidRef = AttributeReference();
    ASSERT_FALSE(kInvalidRef.Valid());

    auto context = ContextBuilder().Kind("user", "userKeyA").Build();
    auto result = Bucket(context, kInvalidRef, kPrefix, false, "user");
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kInvalidAttributeReference);
}

TEST_F(BucketingTests, BucketContextByIntAttribute) {
    auto const kUserKey = "userKeyD";
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context =
        ContextBuilder().Kind("user", kUserKey).Set("intAttr", 33'333).Build();
    auto result = Bucket(context, "intAttr", kPrefix, false, "user");
    ASSERT_TRUE(result) << kUserKey << " should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.54771423, kBucketTolerance);
}

TEST_F(BucketingTests, BucketContextByStringifiedIntAttribute) {
    auto const kUserKey = "userKeyD";
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context = ContextBuilder()
                       .Kind("user", kUserKey)
                       .Set("stringAttr", "33333")
                       .Build();
    auto result = Bucket(context, "stringAttr", kPrefix, false, "user");
    ASSERT_TRUE(result) << kUserKey << " should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.54771423, kBucketTolerance);
}

TEST_F(BucketingTests, BucketContextByFloatAttributeNotAllowed) {
    auto const kUserKey = "userKeyE";
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context = ContextBuilder()
                       .Kind("user", kUserKey)
                       .Set("floatAttr", 999.999)
                       .Build();
    auto result = Bucket(context, "floatAttr", kPrefix, false, "user");
    ASSERT_TRUE(result) << kUserKey << " should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.0, kBucketTolerance);
}

TEST_F(BucketingTests, BucketContextByFloatAttributeThatIsInteger) {
    auto const kUserKey = "userKeyE";
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context = ContextBuilder()
                       .Kind("user", kUserKey)
                       .Set("floatAttr", 33333.0)
                       .Build();
    auto result = Bucket(context, "floatAttr", kPrefix, false, "user");
    ASSERT_TRUE(result) << kUserKey << " should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.54771423, kBucketTolerance);
}

TEST_F(BucketingTests, BucketValueBeyongLastBucketIsPinnedToLastBucket) {
    WeightedVariation wv0(0, 5'000);
    WeightedVariation wv1(1, 5'000);

    Flag::Rollout rollout({wv0, wv1});
    rollout.seed = 61;

    auto context = ContextBuilder()
                       .Kind("user", "userKeyD")
                       .Set("intAttr", 99'999)
                       .Build();

    auto result = Variation(rollout, kHashKey, context, kSalt);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation_index, 1);
    ASSERT_FALSE(result->in_experiment);
}

TEST_F(BucketingTests,
       BucketValueBeyongLastBucketIsPinnedToLastBucketForExperiment) {
    WeightedVariation wv0(0, 5'000);
    WeightedVariation wv1(1, 5'000);

    Flag::Rollout rollout({wv0, wv1});
    rollout.seed = 61;
    rollout.kind = Flag::Rollout::Kind::kExperiment;

    auto context = ContextBuilder()
                       .Kind("user", "userKeyD")
                       .Set("intAttr", 99'999)
                       .Build();

    auto result = Variation(rollout, kHashKey, context, kSalt);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation_index, 1);
    ASSERT_TRUE(result->in_experiment);
}

TEST_F(BucketingTests, IncompleteWeightingDefaultsToLastVariation) {
    WeightedVariation wv0(0, 1);
    WeightedVariation wv1(1, 2);
    WeightedVariation wv2(2, 3);

    Flag::Rollout rollout({wv0, wv1, wv2});

    auto context = ContextBuilder()
                       .Kind("user", "userKeyD")
                       .Set("intAttr", 99'999)
                       .Build();

    auto result = Variation(rollout, kHashKey, context, kSalt);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->variation_index, 2)
        << "userKeyD (bucket 0.7816281) should get variation 2, but got "
        << result->variation_index;
    ASSERT_FALSE(result->in_experiment);
}
