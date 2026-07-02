/** @file dynamodb_big_segment_store.h
 * @brief LaunchDarkly Server-side DynamoDB Big Segments Store C Binding.
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
 * @brief LDServerBigSegmentsDynamoDBStore is a Big Segments persistent store
 * for the Server-Side SDK backed by Amazon DynamoDB. It is meant to be passed
 * to the SDK via the Big Segments config builder.
 *
 * Call @ref LDServerBigSegmentsDynamoDBStore_New to obtain a new instance.
 * This instance is passed into the SDK's Big Segments configuration.
 *
 * The DynamoDB table must already exist and follow the LaunchDarkly schema:
 * a String partition key named `namespace` and a String sort key named
 * `key`. The same table can be shared with @ref LDServerLazyLoadDynamoDBSource
 * -- Big Segments rows occupy their own partition-key values and do not
 * conflict with flag/segment rows. The LaunchDarkly Relay Proxy populates
 * Big Segments data in the table; this store only reads from it.
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
 *  struct LDServerBigSegmentsDynamoDBResult result;
 *
 *  if (!LDServerBigSegmentsDynamoDBStore_New("my-table", "testprefix", options,
 *                                            &result)) {
 *      // On failure, you may print the error message (result.error_message),
 *      // then exit or return.
 *  }
 *
 *  // Create the Big Segments builder, taking ownership of the store pointer.
 *  LDServerBigSegmentsBuilder bs_builder = LDServerBigSegmentsBuilder_New(
 *      (LDServerBigSegmentStorePtr)result.store);
 *
 *  // Attach the Big Segments builder to the SDK config.
 *  LDServerConfigBuilder cfg_builder = LDServerConfigBuilder_New("sdk-123");
 *  LDServerConfigBuilder_BigSegments(cfg_builder, bs_builder);
 * @endcode
 *
 * This implementation is backed by the AWS SDK for C++.
 */
typedef struct _LDServerBigSegmentsDynamoDBStore*
    LDServerBigSegmentsDynamoDBStore;

/* Defines the size of the error message buffer in
 * LDServerBigSegmentsDynamoDBResult.
 */
#ifndef LDSERVER_BIGSEGMENTS_DYNAMODBSTORE_ERROR_MESSAGE_SIZE
#define LDSERVER_BIGSEGMENTS_DYNAMODBSTORE_ERROR_MESSAGE_SIZE 256
#endif

/**
 * @brief Stores the result of calling @ref LDServerBigSegmentsDynamoDBStore_New.
 *
 * On successful creation, store will contain a pointer which may be passed
 * into the LaunchDarkly SDK's Big Segments configuration.
 *
 * On failure, error_message contains a NULL-terminated string describing the
 * error.
 *
 * The message may be truncated if it was originally longer than
 * error_message's buffer size.
 */
struct LDServerBigSegmentsDynamoDBResult {
    LDServerBigSegmentsDynamoDBStore store;
    char error_message[LDSERVER_BIGSEGMENTS_DYNAMODBSTORE_ERROR_MESSAGE_SIZE];
};

/**
 * @brief Creates a new DynamoDB Big Segment store suitable for usage in the
 * SDK's Big Segments configuration.
 *
 * @param table_name Name of the DynamoDB table to read from. The table must
 * already exist; this function does not create it. Must not be NULL.
 *
 * @param prefix Prefix to use when reading Big Segments data from DynamoDB.
 * This allows multiple SDK environments to coexist in the same table. Must
 * not be NULL.
 *
 * @param options Optional AWS DynamoDB client configuration. When non-NULL,
 * the builder is consumed and freed by this function; do not call
 * @ref LDServerDynamoDBClientOptionsBuilder_Free on it afterward. When NULL,
 * the AWS SDK's default provider chain is used for region, endpoint, and
 * credentials.
 *
 * @param out_result Pointer to struct where the store pointer or an error
 * message should be stored. Must not be NULL.
 *
 * @return True if the store was created successfully; out_result->store
 * will contain the pointer. The caller must either free the pointer with
 * @ref LDServerBigSegmentsDynamoDBStore_Free, OR pass it into the SDK's Big
 * Segments configuration which will take ownership (in which case do not
 * call @ref LDServerBigSegmentsDynamoDBStore_Free.)
 */
LD_EXPORT(bool)
LDServerBigSegmentsDynamoDBStore_New(
    char const* table_name,
    char const* prefix,
    LDServerDynamoDBClientOptionsBuilder options,
    struct LDServerBigSegmentsDynamoDBResult* out_result);

/**
 * @brief Frees a DynamoDB Big Segment store pointer. Only necessary to call
 * if not passing ownership to the SDK's Big Segments configuration.
 */
LD_EXPORT(void)
LDServerBigSegmentsDynamoDBStore_Free(LDServerBigSegmentsDynamoDBStore store);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
