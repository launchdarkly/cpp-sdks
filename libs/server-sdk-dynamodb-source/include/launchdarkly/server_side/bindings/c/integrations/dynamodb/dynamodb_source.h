/** @file dynamodb_source.h
 * @brief LaunchDarkly Server-side DynamoDB Source C Binding.
 */
// NOLINTBEGIN modernize-use-using
#pragma once

#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_client_options.h>

#include <launchdarkly/bindings/c/export.h>

#ifdef __cplusplus
extern "C" {
// only need to export C interface if
// used by C++ source code
#endif

/**
 * @brief LDServerLazyLoadDynamoDBSource represents a data source for the
 * Server-Side SDK backed by Amazon DynamoDB. It is meant to be used in place
 * of the standard LaunchDarkly Streaming or Polling data sources.
 *
 * Call @ref LDServerLazyLoadDynamoDBSource_New to obtain a new instance.
 * This instance can be passed into the SDK's DataSystem configuration via
 * the LazyLoad builder.
 *
 * The DynamoDB table must already exist and follow the LaunchDarkly schema:
 * a String partition key named `namespace` and a String sort key named
 * `key`. The LaunchDarkly Relay Proxy populates the table with this schema;
 * this source only reads from it.
 *
 * Example:
 * @code
 *  // Optional: configure the AWS DynamoDB client. Pass NULL for defaults.
 *  LDServerDynamoDBClientOptionsBuilder options =
 *      LDServerDynamoDBClientOptionsBuilder_New();
 *  LDServerDynamoDBClientOptionsBuilder_Region(options, "us-east-1");
 *
 *  // Stack allocate the result struct, which will hold the result pointer or
 *  // an error message.
 *  struct LDServerLazyLoadDynamoDBResult result;
 *
 *  if (!LDServerLazyLoadDynamoDBSource_New("my-table", "testprefix", options,
 *                                          &result)) {
 *      // On failure, you may print the error message (result.error_message),
 *      // then exit or return.
 *  }
 *
 *  LDServerLazyLoadBuilder lazy_builder = LDServerLazyLoadBuilder_New();
 *  LDServerLazyLoadBuilder_SourcePtr(
 *      lazy_builder, (LDServerLazyLoadSourcePtr)result.source);
 *
 *  LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
 *  LDServerConfigBuilder_DataSystem_LazyLoad(cfg_builder, lazy_builder);
 * @endcode
 *
 * This implementation is backed by the AWS SDK for C++.
 */
typedef struct _LDServerLazyLoadDynamoDBSource* LDServerLazyLoadDynamoDBSource;

/* Defines the size of the error message buffer in
 * LDServerLazyLoadDynamoDBResult.
 */
#ifndef LDSERVER_LAZYLOAD_DYNAMODBSOURCE_ERROR_MESSAGE_SIZE
#define LDSERVER_LAZYLOAD_DYNAMODBSOURCE_ERROR_MESSAGE_SIZE 256
#endif

/**
 * @brief Stores the result of calling @ref LDServerLazyLoadDynamoDBSource_New.
 *
 * On successful creation, source will contain a pointer which may be passed
 * into the LaunchDarkly SDK's configuration.
 *
 * On failure, error_message contains a NULL-terminated string describing the
 * error.
 *
 * The message may be truncated if it was originally longer than
 * error_message's buffer size.
 */
struct LDServerLazyLoadDynamoDBResult {
    LDServerLazyLoadDynamoDBSource source;
    char error_message[LDSERVER_LAZYLOAD_DYNAMODBSOURCE_ERROR_MESSAGE_SIZE];
};

/**
 * @brief Creates a new DynamoDB data source suitable for usage in the SDK's
 * Lazy Load data system.
 *
 * In this system, the SDK will query DynamoDB for flag/segment data as
 * required, with an in-memory cache to reduce the number of queries.
 *
 * Data is never written back to DynamoDB by the SDK; the LaunchDarkly Relay
 * Proxy populates the table.
 *
 * @param table_name Name of the DynamoDB table to read from. The table must
 * already exist; this function does not create it. Must not be NULL.
 *
 * @param prefix Prefix to use when reading SDK data from DynamoDB. This
 * allows multiple SDK environments to coexist in the same table. Must not
 * be NULL.
 *
 * @param options Optional AWS DynamoDB client configuration. When non-NULL,
 * the builder is consumed and freed by this function; do not call
 * @ref LDServerDynamoDBClientOptionsBuilder_Free on it afterward. When NULL,
 * the AWS SDK's default provider chain is used for region, endpoint, and
 * credentials.
 *
 * @param out_result Pointer to struct where the source pointer or an error
 * message should be stored. Must not be NULL.
 *
 * @return True if the source was created successfully; out_result->source
 * will contain the pointer. The caller must either free the pointer with
 * @ref LDServerLazyLoadDynamoDBSource_Free, OR pass it into the SDK's
 * configuration methods which will take ownership (in which case do not
 * call @ref LDServerLazyLoadDynamoDBSource_Free.)
 */
LD_EXPORT(bool)
LDServerLazyLoadDynamoDBSource_New(
    char const* table_name,
    char const* prefix,
    LDServerDynamoDBClientOptionsBuilder options,
    struct LDServerLazyLoadDynamoDBResult* out_result);

/**
 * @brief Frees a DynamoDB data source pointer. Only necessary to call if not
 * passing ownership to SDK configuration.
 */
LD_EXPORT(void)
LDServerLazyLoadDynamoDBSource_Free(LDServerLazyLoadDynamoDBSource source);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
