#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_big_segment_store.h>

#include <launchdarkly/server_side/integrations/redis/redis_big_segment_store.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <cstring>

using namespace launchdarkly::server_side::integrations;

LD_EXPORT(bool)
LDServerBigSegmentsRedisStore_New(char const* uri,
                                  char const* prefix,
                                  LDServerBigSegmentsRedisResult* out_result) {
    LD_ASSERT_NOT_NULL(uri);
    LD_ASSERT_NOT_NULL(prefix);
    LD_ASSERT_NOT_NULL(out_result);

    // Explicitely zero out the exception_msg buffer in case the exception is
    // shorter than the buffer.
    memset(out_result->error_message, 0,
           sizeof(LDServerBigSegmentsRedisResult::error_message));

    // Ensure the store pointer isn't garbage.
    out_result->store = nullptr;

    auto maybe_store = RedisBigSegmentStore::Create(uri, prefix);
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
    out_result->store =
        reinterpret_cast<LDServerBigSegmentsRedisStore>(maybe_store->release());

    return true;
}

LD_EXPORT(void)
LDServerBigSegmentsRedisStore_Free(LDServerBigSegmentsRedisStore store) {
    delete reinterpret_cast<RedisBigSegmentStore*>(store);
}
