#include "evaluation/bucketing.hpp"

#include <gtest/gtest.h>
#include <launchdarkly/context_builder.hpp>

using namespace launchdarkly;
using namespace launchdarkly::server_side::evaluation;

/**
 * Note: These tests are meant to be exact duplicates of tests
 * in other SDKs. Do not change any of the values unless they
 * are also changed in other SDKs. These are not traditional behavioral
 * tests so much as consistency tests to guarantee that the implementation
 * is identical across SDKs.
 *
 * Tests in this file may derive from BucketingConsistencyTests to gain access
 * to shared constants.
 */
class BucketingConsistencyTests : public ::testing::Test {
   public:
    // Bucket results must be no more than this distance from the expected
    // value.
    double const kBucketTolerance = 0.0000001;
    const std::string kHashKey = "hashKey";
    const std::string kSalt = "saltyA";
};

TEST_F(BucketingConsistencyTests, BucketContextByKey) {
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

TEST_F(BucketingConsistencyTests, BucketContextByKeyWithSeed) {
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

TEST_F(BucketingConsistencyTests, BucketContextByInvalidReference) {
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);
    auto const kInvalidRef = AttributeReference();
    ASSERT_FALSE(kInvalidRef.Valid());

    auto context = ContextBuilder().Kind("user", "userKeyA").Build();
    auto result = Bucket(context, kInvalidRef, kPrefix, false, "user");
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), Error::kInvalidAttributeReference);
}

TEST_F(BucketingConsistencyTests, BucketContextByIntAttribute) {
    auto const kUserKey = "userKeyD";
    auto const kPrefix = BucketPrefix(kHashKey, kSalt);

    auto context =
        ContextBuilder().Kind("user", kUserKey).Set("intAttr", 33'333).Build();
    auto result = Bucket(context, "intAttr", kPrefix, false, "user");
    ASSERT_TRUE(result) << kUserKey << " should be bucketed but got "
                        << result.error();
    ASSERT_NEAR(result->first, 0.54771423, kBucketTolerance);
}

TEST_F(BucketingConsistencyTests, BucketContextByStringifiedIntAttribute) {
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

TEST_F(BucketingConsistencyTests, BucketContextByFloatAttributeNotAllowed) {
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

TEST_F(BucketingConsistencyTests, BucketContextByFloatAttributeThatIsInteger) {
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
