#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_big_segment_store.h>

#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_big_segment_store.hpp>
#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <cstring>
#include <utility>

using namespace launchdarkly::server_side::integrations;

LD_EXPORT(bool)
LDServerBigSegmentsDynamoDBStore_New(
    char const* table_name,
    char const* prefix,
    LDServerDynamoDBClientOptionsBuilder options,
    LDServerBigSegmentsDynamoDBResult* out_result) {
    LD_ASSERT_NOT_NULL(table_name);
    LD_ASSERT_NOT_NULL(prefix);
    LD_ASSERT_NOT_NULL(out_result);

    // Explicitely zero out the error_message buffer in case the error is
    // shorter than the buffer.
    memset(out_result->error_message, 0,
           sizeof(LDServerBigSegmentsDynamoDBResult::error_message));

    // Ensure the store pointer isn't garbage.
    out_result->store = nullptr;

    DynamoDBClientOptions opts{};
    if (options != nullptr) {
        auto* opts_ptr = reinterpret_cast<DynamoDBClientOptions*>(options);
        opts = *opts_ptr;
        LDServerDynamoDBClientOptionsBuilder_Free(options);
    }

    auto maybe_store =
        DynamoDBBigSegmentStore::Create(table_name, prefix, std::move(opts));
    if (!maybe_store) {
        // Avoid heap allocating another string to pass back to the caller;
        // instead, we copy into the buffer and ensure a terminator is present.
        // This does mean the message may be truncated.
        std::size_t const len = maybe_store.error().copy(
            out_result->error_message, sizeof(out_result->error_message) - 1);
        out_result->error_message[len] = '\0';
        return false;
    }

    // The pointer is no longer managed and must either be freed by the caller,
    // or passed into the SDK which will take ownership.
    out_result->store = reinterpret_cast<LDServerBigSegmentsDynamoDBStore>(
        maybe_store->release());
    return true;
}

LD_EXPORT(void)
LDServerBigSegmentsDynamoDBStore_Free(LDServerBigSegmentsDynamoDBStore store) {
    delete reinterpret_cast<DynamoDBBigSegmentStore*>(store);
}
