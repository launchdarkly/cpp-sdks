#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_big_segment_store.hpp>

#include "aws_sdk_guard.hpp"
#include "prefixed_dynamodb_client.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>
#include <aws/dynamodb/DynamoDBClient.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>

using namespace launchdarkly::server_side::integrations;

namespace {

std::string EnvOr(char const* name, std::string const& fallback) {
    char const* value = std::getenv(name);
    if (value && *value) {
        return value;
    }
    return fallback;
}

DynamoDBClientOptions LocalOptions() {
    DynamoDBClientOptions options;
    options.endpoint =
        EnvOr("LD_DYNAMODB_TEST_ENDPOINT", "http://localhost:8000");
    options.region = EnvOr("LD_DYNAMODB_TEST_REGION", "us-east-1");
    options.aws_access_key_id = "dummy";
    options.aws_secret_access_key = "dummy";
    return options;
}

class DynamoDBBigSegmentTests : public ::testing::Test {
   public:
    DynamoDBBigSegmentTests()
        : table_name_("ld-dynamodb-big-segments-test"),
          prefix_("testprefix"),
          options_(LocalOptions()),
          client_(MakeRawClient()) {}

    void SetUp() override {
        PrefixedDynamoDBClient::DeleteTable(*client_, table_name_);
        PrefixedDynamoDBClient::CreateTable(*client_, table_name_);

        auto maybe_store =
            DynamoDBBigSegmentStore::Create(table_name_, prefix_, options_);
        ASSERT_TRUE(maybe_store) << maybe_store.error();
        store_ = std::move(*maybe_store);
    }

    void TearDown() override {
        store_.reset();
        PrefixedDynamoDBClient::DeleteTable(*client_, table_name_);
    }

   protected:
    std::unique_ptr<DynamoDBBigSegmentStore> store_;
    std::string const table_name_;
    std::string const prefix_;
    DynamoDBClientOptions const options_;
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client_;

   private:
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> MakeRawClient() const {
        detail::AwsSdkGuard::Ensure();
        Aws::Client::ClientConfiguration config;
        config.region = *options_.region;
        config.endpointOverride = *options_.endpoint;
        if (options_.endpoint->rfind("http://", 0) == 0) {
            config.scheme = Aws::Http::Scheme::HTTP;
            config.verifySSL = false;
        }
        Aws::Auth::AWSCredentials creds{*options_.aws_access_key_id,
                                        *options_.aws_secret_access_key};
        return std::make_unique<Aws::DynamoDB::DynamoDBClient>(creds, config);
    }
};

}  // namespace

TEST_F(DynamoDBBigSegmentTests, EmptyStoreReturnsEmptyMembership) {
    auto const result = store_->GetMembership("nobody");
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->CheckMembership("seg1.g1").has_value());
}

TEST_F(DynamoDBBigSegmentTests, EmptyStoreReturnsNoMetadata) {
    auto const result = store_->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->has_value());
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipWithIncludesOnly) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutBigSegmentMembership("alice", {"seg1.g1", "seg2.g3"}, {});

    auto const result = store_->GetMembership("alice");
    ASSERT_TRUE(result);

    ASSERT_EQ(result->CheckMembership("seg1.g1"), true);
    ASSERT_EQ(result->CheckMembership("seg2.g3"), true);
    ASSERT_FALSE(result->CheckMembership("seg3.g1").has_value());
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipWithExcludesOnly) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutBigSegmentMembership("bob", {}, {"seg1.g1"});

    auto const result = store_->GetMembership("bob");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg1.g1"), false);
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipInclusionWinsOverExclusion) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutBigSegmentMembership("carol", {"seg.g1"}, {"seg.g1"});

    auto const result = store_->GetMembership("carol");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg.g1"), true);
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipIsPrefixScoped) {
    PrefixedDynamoDBClient(*client_, "otherprefix", table_name_)
        .PutBigSegmentMembership("alice", {"seg1.g1"}, {});

    auto const result = store_->GetMembership("alice");
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->CheckMembership("seg1.g1").has_value());
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipWithEmptyPrefix) {
    auto maybe_store =
        DynamoDBBigSegmentStore::Create(table_name_, "", options_);
    ASSERT_TRUE(maybe_store) << maybe_store.error();
    auto const store = std::move(*maybe_store);

    PrefixedDynamoDBClient(*client_, "", table_name_)
        .PutBigSegmentMembership("alice", {"seg1.g1"}, {});

    auto const result = store->GetMembership("alice");
    ASSERT_TRUE(result);
    ASSERT_EQ(result->CheckMembership("seg1.g1"), true);
}

TEST_F(DynamoDBBigSegmentTests, GetMetadataWithEmptyPrefix) {
    auto maybe_store =
        DynamoDBBigSegmentStore::Create(table_name_, "", options_);
    ASSERT_TRUE(maybe_store) << maybe_store.error();
    auto const store = std::move(*maybe_store);

    PrefixedDynamoDBClient(*client_, "", table_name_)
        .PutBigSegmentSyncTime(1700000000000LL);

    auto const result = store->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    ASSERT_EQ(result->value().last_up_to_date,
              std::chrono::milliseconds{1700000000000LL});
}

TEST_F(DynamoDBBigSegmentTests, GetMembershipRejectsMalformedIncluded) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutMalformedBigSegmentMembership("dave");

    auto const result = store_->GetMembership("dave");
    ASSERT_FALSE(result);
}

TEST_F(DynamoDBBigSegmentTests, GetMetadataReturnsSyncTime) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutBigSegmentSyncTime(1700000000000LL);

    auto const result = store_->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->has_value());
    ASSERT_EQ(result->value().last_up_to_date,
              std::chrono::milliseconds{1700000000000LL});
}

TEST_F(DynamoDBBigSegmentTests, GetMetadataAbsentSyncTimeReturnsNoMetadata) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutBigSegmentMetadataWithoutSyncTime();

    auto const result = store_->GetMetadata();
    ASSERT_TRUE(result);
    ASSERT_FALSE(result->has_value());
}

TEST_F(DynamoDBBigSegmentTests, GetMetadataRejectsMalformedSyncTime) {
    PrefixedDynamoDBClient(*client_, prefix_, table_name_)
        .PutMalformedBigSegmentSyncTime();

    auto const result = store_->GetMetadata();
    ASSERT_FALSE(result);
}
