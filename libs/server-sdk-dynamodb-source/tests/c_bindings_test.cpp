#include <gtest/gtest.h>

#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_client_options.h>
#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_source.h>

#include <launchdarkly/bindings/c/context_builder.h>
#include <launchdarkly/server_side/bindings/c/config/builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>

#include <launchdarkly/data_model/flag.hpp>

#include "aws_sdk_guard.hpp"
#include "prefixed_dynamodb_client.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>
#include <aws/dynamodb/DynamoDBClient.h>

#include <cstdlib>
#include <memory>
#include <string>

using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::integrations;

namespace {

std::string EnvOr(char const* name, std::string const& fallback) {
    char const* value = std::getenv(name);
    if (value && *value) {
        return value;
    }
    return fallback;
}

std::string LocalEndpoint() {
    return EnvOr("LD_DYNAMODB_TEST_ENDPOINT", "http://localhost:8000");
}

std::string LocalRegion() {
    return EnvOr("LD_DYNAMODB_TEST_REGION", "us-east-1");
}

std::unique_ptr<Aws::DynamoDB::DynamoDBClient> MakeRawClient() {
    detail::AwsSdkGuard::Ensure();
    Aws::Client::ClientConfiguration config;
    config.region = LocalRegion();
    config.endpointOverride = LocalEndpoint();
    if (config.endpointOverride.rfind("http://", 0) == 0) {
        config.scheme = Aws::Http::Scheme::HTTP;
        config.verifySSL = false;
    }
    Aws::Auth::AWSCredentials creds("dummy", "dummy");
    return std::make_unique<Aws::DynamoDB::DynamoDBClient>(creds, config);
}

LDServerDynamoDBClientOptionsBuilder LocalOptionsBuilder() {
    LDServerDynamoDBClientOptionsBuilder opts =
        LDServerDynamoDBClientOptionsBuilder_New();
    LDServerDynamoDBClientOptionsBuilder_Region(opts, LocalRegion().c_str());
    LDServerDynamoDBClientOptionsBuilder_Endpoint(opts,
                                                  LocalEndpoint().c_str());
    LDServerDynamoDBClientOptionsBuilder_AccessKeyId(opts, "dummy");
    LDServerDynamoDBClientOptionsBuilder_SecretAccessKey(opts, "dummy");
    return opts;
}

}  // namespace

TEST(DynamoDBBindings, OptionsBuilderCanBeCreatedAndFreed) {
    LDServerDynamoDBClientOptionsBuilder opts =
        LDServerDynamoDBClientOptionsBuilder_New();
    ASSERT_NE(opts, nullptr);
    LDServerDynamoDBClientOptionsBuilder_Region(opts, "us-east-1");
    LDServerDynamoDBClientOptionsBuilder_Endpoint(opts,
                                                  "http://localhost:8000");
    LDServerDynamoDBClientOptionsBuilder_AccessKeyId(opts, "id");
    LDServerDynamoDBClientOptionsBuilder_SecretAccessKey(opts, "secret");
    LDServerDynamoDBClientOptionsBuilder_SessionToken(opts, "token");
    LDServerDynamoDBClientOptionsBuilder_Free(opts);
}

TEST(DynamoDBBindings, LazyLoadSourcePointerIsStoredOnSuccessfulCreation) {
    LDServerLazyLoadDynamoDBResult result;
    ASSERT_TRUE(LDServerLazyLoadDynamoDBSource_New(
        "any-table", "foo", LocalOptionsBuilder(), &result));
    ASSERT_NE(result.source, nullptr);
    LDServerLazyLoadDynamoDBSource_Free(result.source);
}

TEST(DynamoDBBindings, LazyLoadSourceAcceptsNullOptions) {
    LDServerLazyLoadDynamoDBResult result;
    ASSERT_TRUE(LDServerLazyLoadDynamoDBSource_New("any-table", "foo", nullptr,
                                                   &result));
    ASSERT_NE(result.source, nullptr);
    LDServerLazyLoadDynamoDBSource_Free(result.source);
}

// End-to-end test that uses an actual DynamoDB (Local) instance with
// provisioned flag data. The source is passed into the SDK's LazyLoad data
// system, and AllFlags is used to verify that the data is read back from
// DynamoDB correctly through the C binding.
TEST(DynamoDBBindings, CanUseInSDKLazyLoadDataSource) {
    std::string const table_name = "ld-dynamodb-c-bindings-test";
    std::string const prefix = "testprefix";

    auto raw_client = MakeRawClient();
    PrefixedDynamoDBClient::DeleteTable(*raw_client, table_name);
    PrefixedDynamoDBClient::CreateTable(*raw_client, table_name);

    PrefixedDynamoDBClient client(*raw_client, prefix, table_name);
    Flag flag_a{"foo", 1, false, std::nullopt, {true, false}};
    flag_a.offVariation = 0;
    Flag flag_b{"bar", 1, false, std::nullopt, {true, false}};
    flag_b.offVariation = 1;

    client.PutFlag(flag_a);
    client.PutFlag(flag_b);
    client.Init();

    LDServerLazyLoadDynamoDBResult result;
    ASSERT_TRUE(LDServerLazyLoadDynamoDBSource_New(
        table_name.c_str(), prefix.c_str(), LocalOptionsBuilder(), &result));

    LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");

    LDServerLazyLoadBuilder lazy_builder = LDServerLazyLoadBuilder_New();
    LDServerLazyLoadBuilder_SourcePtr(
        lazy_builder,
        reinterpret_cast<LDServerLazyLoadSourcePtr>(result.source));
    LDServerConfigBuilder_DataSystem_LazyLoad(cfg_builder, lazy_builder);
    LDServerConfigBuilder_Events_Enabled(cfg_builder, false);

    LDServerConfig config;
    LDStatus status = LDServerConfigBuilder_Build(cfg_builder, &config);
    ASSERT_TRUE(LDStatus_Ok(status));

    LDServerSDK sdk = LDServerSDK_New(config);
    LDServerSDK_Start(sdk, LD_NONBLOCKING, nullptr);

    LDContextBuilder ctx_builder = LDContextBuilder_New();
    LDContextBuilder_AddKind(ctx_builder, "cat", "shadow");
    LDContext context = LDContextBuilder_Build(ctx_builder);

    LDAllFlagsState state =
        LDServerSDK_AllFlagsState(sdk, context, LD_ALLFLAGSSTATE_DEFAULT);

    ASSERT_TRUE(LDAllFlagsState_Valid(state));
    LDValue all = LDAllFlagsState_Map(state);
    ASSERT_EQ(LDValue_Type(all), LDValueType_Object);

    std::unordered_map<std::string, launchdarkly::Value> values;
    LDValue_ObjectIter iter;
    for (iter = LDValue_ObjectIter_New(all); !LDValue_ObjectIter_End(iter);
         LDValue_ObjectIter_Next(iter)) {
        char const* key = LDValue_ObjectIter_Key(iter);
        auto value_ref = reinterpret_cast<launchdarkly::Value const* const>(
            LDValue_ObjectIter_Value(iter));
        values.emplace(key, *value_ref);
    }

    LDValue_ObjectIter_Free(iter);

    std::unordered_map<std::string, launchdarkly::Value> expected = {
        {"foo", true}, {"bar", false}};
    ASSERT_EQ(values, expected);

    LDValue_Free(all);
    LDAllFlagsState_Free(state);

    LDContext_Free(context);
    LDServerSDK_Free(sdk);

    PrefixedDynamoDBClient::DeleteTable(*raw_client, table_name);
}
