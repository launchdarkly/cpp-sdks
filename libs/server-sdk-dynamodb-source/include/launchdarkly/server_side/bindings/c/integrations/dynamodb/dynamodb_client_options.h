/** @file dynamodb_client_options.h
 * @brief LaunchDarkly Server-side DynamoDB Client Options C Binding.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

/**
 * @brief LDServerDynamoDBClientOptionsBuilder configures the AWS DynamoDB
 * client that a LaunchDarkly DynamoDB integration will use.
 *
 * All fields are optional. When left unset, the AWS SDK's default provider
 * chain (environment variables, shared config, EC2/ECS instance metadata) is
 * used to resolve the corresponding field.
 *
 * The builder is passed by handle to a DynamoDB source or store factory,
 * which takes ownership and frees it. Callers only need to call
 * @ref LDServerDynamoDBClientOptionsBuilder_Free directly if the builder is
 * not passed to a factory.
 */
typedef struct _LDServerDynamoDBClientOptionsBuilder*
    LDServerDynamoDBClientOptionsBuilder;

/**
 * @brief Creates a new DynamoDB client options builder with all fields unset.
 *
 * @return A new builder handle.
 */
LD_EXPORT(LDServerDynamoDBClientOptionsBuilder)
LDServerDynamoDBClientOptionsBuilder_New(void);

/**
 * @brief Sets the AWS region for the DynamoDB client.
 *
 * When unset, the AWS SDK resolves the region via the standard region
 * provider chain (environment variables, shared config file, instance
 * metadata).
 *
 * @param b Builder. Must not be NULL.
 * @param region Region string (e.g. "us-east-1"). Must not be NULL.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Region(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* region);

/**
 * @brief Sets a custom endpoint for the DynamoDB client.
 *
 * Useful when pointing at DynamoDB Local or LocalStack for testing (e.g.
 * "http://localhost:8000"). When unset, the AWS SDK uses the standard
 * DynamoDB endpoint for the resolved region.
 *
 * @param b Builder. Must not be NULL.
 * @param endpoint Endpoint URL. Must not be NULL.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Endpoint(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* endpoint);

/**
 * @brief Sets the AWS access key ID for the DynamoDB client.
 *
 * When none of AccessKeyId, SecretAccessKey, or SessionToken are set, the
 * AWS SDK's default credential provider chain is used (environment
 * variables, shared credentials file, EC2/ECS roles).
 *
 * @param b Builder. Must not be NULL.
 * @param access_key_id AWS access key ID. Must not be NULL.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_AccessKeyId(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* access_key_id);

/**
 * @brief Sets the AWS secret access key for the DynamoDB client.
 *
 * @param b Builder. Must not be NULL.
 * @param secret_access_key AWS secret access key. Must not be NULL.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_SecretAccessKey(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* secret_access_key);

/**
 * @brief Sets the AWS session token for the DynamoDB client (for temporary
 * credentials).
 *
 * @param b Builder. Must not be NULL.
 * @param session_token AWS session token. Must not be NULL.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_SessionToken(
    LDServerDynamoDBClientOptionsBuilder b,
    char const* session_token);

/**
 * @brief Frees a DynamoDB client options builder. Do not call if the builder
 * was consumed by a DynamoDB source or store factory.
 *
 * @param b Builder to free.
 */
LD_EXPORT(void)
LDServerDynamoDBClientOptionsBuilder_Free(
    LDServerDynamoDBClientOptionsBuilder b);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
