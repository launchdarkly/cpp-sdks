#include <launchdarkly/server_side/bindings/c/integrations/dynamodb/dynamodb_source.h>

#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_source.hpp>
#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <cstring>
#include <utility>

using namespace launchdarkly::server_side::integrations;

LD_EXPORT(bool)
LDServerLazyLoadDynamoDBSource_New(char const* table_name,
                                   char const* prefix,
                                   LDServerDynamoDBClientOptionsBuilder options,
                                   LDServerLazyLoadDynamoDBResult* out_result) {
    LD_ASSERT_NOT_NULL(table_name);
    LD_ASSERT_NOT_NULL(prefix);
    LD_ASSERT_NOT_NULL(out_result);

    // Explicitely zero out the error_message buffer in case the error is
    // shorter than the buffer.
    memset(out_result->error_message, 0,
           sizeof(LDServerLazyLoadDynamoDBResult::error_message));

    // Ensure the source pointer isn't garbage.
    out_result->source = nullptr;

    DynamoDBClientOptions opts{};
    if (options != nullptr) {
        auto* opts_ptr = reinterpret_cast<DynamoDBClientOptions*>(options);
        opts = *opts_ptr;
        LDServerDynamoDBClientOptionsBuilder_Free(options);
    }

    auto maybe_source =
        DynamoDBDataSource::Create(table_name, prefix, std::move(opts));
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
    out_result->source = reinterpret_cast<LDServerLazyLoadDynamoDBSource>(
        maybe_source->release());
    return true;
}

LD_EXPORT(void)
LDServerLazyLoadDynamoDBSource_Free(LDServerLazyLoadDynamoDBSource source) {
    delete reinterpret_cast<DynamoDBDataSource*>(source);
}
