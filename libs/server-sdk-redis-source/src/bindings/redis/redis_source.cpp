#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h>

#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/error.hpp>

#include <redis++.h>

using namespace launchdarkly::server_side::integrations;

LD_EXPORT(bool)
LDServerLazyLoadRedisSource_New(char const* uri,
                                char const* prefix,
                                LDServerLazyLoadRedisResult* out_result) {
    LD_ASSERT_NOT_NULL(uri);
    LD_ASSERT_NOT_NULL(prefix);
    LD_ASSERT_NOT_NULL(out_result);

    // Explicitely zero out the exception_msg buffer in case the exception is
    // shorter than the buffer.
    memset(out_result->error_message, 0,
           sizeof(LDServerLazyLoadRedisResult::error_message));

    // Ensure the source pointer isn't garbage.
    out_result->source = nullptr;

    auto maybe_source = RedisDataSource::Create(uri, prefix);
    if (!maybe_source) {
        // Avoid heap allocating another string to pass back to the caller;
        // instead, we copy into the buffer and ensure a terminator is present.
        // This does mean the message may be truncated.

        std::size_t const len = maybe_source.error().copy(
            out_result->error_message, sizeof(out_result->error_message) - 1);
        out_result->error_message[len] = '\0';

        return false;
    }

    // The pointer is no longer managed and must either be freed by the caller,
    // or passed into the SDK which will take ownership.
    out_result->source =
        reinterpret_cast<LDServerLazyLoadRedisSource>(maybe_source->release());

    return true;
}

LD_EXPORT(void)
LDServerLazyLoadRedisSource_Free(LDServerLazyLoadRedisSource source) {
    delete reinterpret_cast<RedisDataSource*>(source);
}
