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

/**
 * Tests in this file may derive from BucketingTests to gain access to shared
 * constants.
 */
class BucketingTests : public ::testing::Test {
   public:
    // Bucket results must be no more than this distance from the expected
    // value.
    const static double kBucketTolerance;
    // An arbitrary hash key.
    const static std::string kHashKey;
    // An arbitrary salt.
    const static std::string kSalt;
};

double const BucketingTests::kBucketTolerance = 0.0000001;
std::string const BucketingTests::kHashKey = "hashKey";
std::string const BucketingTests::kSalt = "saltyA";

// Parameterized tests may be instantiated with one or more BucketTests for
// convenience.
struct BucketTest {
    // Context key.
    std::string key;
    // Expected bucket value as a string; this is only used for printing on
    // error.
    std::string expectedBucket;
    // Expected computed variation index.
    Flag::Variation expectedVariation;
    // Whether the context was determined to be in an experiment.
    bool expectedInExperiment;
    // The rollout used for the test, which may be a percent rollout or an
    // experiment.
    Flag::Rollout rollout;
};

#define IN_EXPERIMENT true
#define NOT_IN_EXPERIMENT false

class BucketVariationTest : public BucketingTests,
                            public ::testing::WithParamInterface<BucketTest> {};

static Flag::Rollout PercentRollout() {
    WeightedVariation wv0(0, 60'000);
    WeightedVariation wv1(1, 40'000);
    return Flag::Rollout({wv0, wv1});
}

static Flag::Rollout ExperimentRollout() {
    auto wv0 = WeightedVariation(0, 10'000);
    auto wv1 = WeightedVariation(1, 20'000);
    auto wv0_untracked = WeightedVariation::Untracked(0, 70'000);
    Flag::Rollout rollout({wv0, wv1, wv0_untracked});
    rollout.kind = Flag::Rollout::Kind::kExperiment;
    rollout.seed = 61;
    return rollout;
}

static Flag::Rollout IncompleteWeighting() {
    WeightedVariation wv0(0, 1);
    WeightedVariation wv1(1, 2);
    WeightedVariation wv2(2, 3);

    return Flag::Rollout({wv0, wv1, wv2});
}

TEST_P(BucketVariationTest, VariationIndexForContext) {
    auto const& param = GetParam();

    auto result =
        Variation(param.rollout, kHashKey,
                  ContextBuilder().Kind("user", param.key).Build(), kSalt);

    ASSERT_TRUE(result) << param.key
                        << " should be assigned a bucket result, but got: "
                        << result.error();

    ASSERT_EQ(result->variation_index, param.expectedVariation)
        << param.key << " (bucket " << param.expectedBucket
        << ") should get variation " << param.expectedVariation << ", but got "
        << result->variation_index;

    ASSERT_EQ(result->in_experiment, param.expectedInExperiment)
        << param.key << " "
        << (param.expectedInExperiment ? "should" : "should not")
        << " be in experiment";
}

INSTANTIATE_TEST_SUITE_P(
    PercentRollout,
    BucketVariationTest,
    ::testing::ValuesIn({BucketTest{"userKeyA", "0.42157587", 0,
                                    NOT_IN_EXPERIMENT, PercentRollout()},
                         BucketTest{"userKeyB", "0.6708485", 1,
                                    NOT_IN_EXPERIMENT, PercentRollout()},
                         BucketTest{"userKeyC", "0.10343106", 0,
                                    NOT_IN_EXPERIMENT, PercentRollout()}}));

INSTANTIATE_TEST_SUITE_P(ExperimentRollout,
                         BucketVariationTest,
                         ::testing::ValuesIn({
                             BucketTest{"userKeyA", "0.09801207", 0,
                                        IN_EXPERIMENT, ExperimentRollout()},
                             BucketTest{"userKeyB", "0.14483777", 1,
                                        IN_EXPERIMENT, ExperimentRollout()},
                             BucketTest{"userKeyC", "0.9242641", 0,
                                        NOT_IN_EXPERIMENT, ExperimentRollout()},
                         }));

INSTANTIATE_TEST_SUITE_P(IncompleteWeightingDefaultsToLastVariation,
                         BucketVariationTest,
                         ::testing::ValuesIn({
                             BucketTest{"userKeyD", "0.7816281", 2,
                                        NOT_IN_EXPERIMENT,
                                        IncompleteWeighting()},
                         }));

#undef IN_EXPERIMENT
#undef NOT_IN_EXPERIMENT

TEST_F(BucketingTests, VariationIndexForContextWithCustomAttribute) {
    WeightedVariation wv0(0, 60'000);
    WeightedVariation wv1(1, 40'000);

    Flag::Rollout rollout({wv0, wv1});
    rollout.bucketBy = "intAttr";

    auto tests = std::vector<std::tuple<int, int, char const*>>{
        {33'333, 0, "0.54771423"}, {99'999, 1, "0.7309658"}};

    for (auto [bucketAttr, expectedVariation, expectedBucket] : tests) {
        auto result = Variation(rollout, kHashKey,
                                ContextBuilder()
                                    .Kind("user", "userKeyA")
                                    .Set("intAttr", bucketAttr)
                                    .Build(),
                                kSalt);

        ASSERT_TRUE(result)
            << "userKeyA should be assigned a bucket result, but got: "
            << result.error();

        ASSERT_EQ(result->variation_index, expectedVariation)
            << "userKeyA (bucket " << expectedBucket
            << ") should get variation " << expectedVariation << ", but got "
            << result->variation_index;

        ASSERT_EQ(result->in_experiment, false)
            << "userKeyA should not be in experiment";
    }
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

    auto tests =
        std::vector<std::pair<std::string, double>>{{"userKeyA", 0.42157587},
                                                    {"userKeyB", 0.6708485},
                                                    {"userKeyC", 0.10343106}};

    for (auto [key, bucket] : tests) {
        auto context = ContextBuilder().Kind("user", key).Build();
        auto result = Bucket(context, "key", kPrefix, false, "user");
        ASSERT_TRUE(result)
            << key << " should be bucketed but got " << result.error();

        ASSERT_NEAR(result->first, bucket, kBucketTolerance);
    }
}

TEST_F(BucketingTests, BucketContextByKeyWithSeed) {
    auto const kPrefix = BucketPrefix::Seed(61);

    auto tests =
        std::vector<std::pair<std::string, double>>{{"userKeyA", 0.09801207},
                                                    {"userKeyB", 0.14483777},
                                                    {"userKeyC", 0.9242641}};

    for (auto [key, bucket] : tests) {
        auto context = ContextBuilder().Kind("user", key).Build();
        auto result = Bucket(context, "key", kPrefix, false, "user");
        ASSERT_TRUE(result)
            << key << " should be bucketed but got " << result.error();

        ASSERT_NEAR(result->first, bucket, kBucketTolerance);

        auto result_diff_seed =
            Bucket(context, "key", BucketPrefix::Seed(60), false, "user");
        ASSERT_TRUE(result_diff_seed) << key << " should be bucketed but got "
                                      << result_diff_seed.error();

        ASSERT_NE(result_diff_seed->first, result->first);
    }
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

TEST_F(BucketingTests, BucketValueBeyondLastBucketIsPinnedToLastBucket) {
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
