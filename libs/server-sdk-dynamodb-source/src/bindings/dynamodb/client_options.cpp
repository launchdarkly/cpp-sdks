#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_client_options.h>

#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

using namespace launchdarkly::server_side::integrations;

#define TO_OPTIONS(ptr) (reinterpret_cast<DynamoDBClientOptions*>(ptr))
#define FROM_OPTIONS(ptr) \
    (reinterpret_cast<LDServerDynamoDBClientOptionsBuilder>(ptr))

LD_EXPORT(LDServerDynamoDBClientOptionsBuilder)
LDServerDynamoDBClientOptionsBuilder_New(void) {
    return FROM_OPTIONS(new DynamoDBClientOptions{});
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Region(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* region) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(region);
    TO_OPTIONS(b)->region = region;
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Endpoint(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* endpoint) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(endpoint);
    TO_OPTIONS(b)->endpoint = endpoint;
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_AccessKeyId(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* access_key_id) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(access_key_id);
    TO_OPTIONS(b)->aws_access_key_id = access_key_id;
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_SecretAccessKey(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* secret_access_key) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(secret_access_key);
    TO_OPTIONS(b)->aws_secret_access_key = secret_access_key;
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_SessionToken(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* session_token) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(session_token);
    TO_OPTIONS(b)->aws_session_token = session_token;
}

LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Free(
    LDServerDynamoDBClientOptionsBuilder b) {
    delete TO_OPTIONS(b);
}
