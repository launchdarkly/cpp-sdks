#include <launchdarkly/server_side/bindings/c/integrations/redis/redis_source.h>

#include <launchdarkly/server_side/integrations/redis/redis_source.hpp>

#include <launchdarkly/error.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

using namespace launchdarkly::server_side::integrations;

LD_EXPORT(LDStatus) LDServerSDK_RedisSource_Create(
    const char* uri,
    const char* prefix,
    LDServerSDK_RedisSource* out_source,
    char** out_exception_msg) {
    LD_ASSERT_NOT_NULL(uri);
    LD_ASSERT_NOT_NULL(prefix);
    LD_ASSERT_NOT_NULL(out_source);

    auto maybe_source = RedisDataSource::Create(uri, prefix);
    if (!maybe_source) {
        if (out_exception_msg) {
            *out_exception_msg = strdup(maybe_source.error().c_str());
        }
        return reinterpret_cast<LDStatus>(new launchdarkly::Error(
            launchdarkly::Error::k));
    }

    *out_source = reinterpret_cast<LDServerSDK_RedisSource>(new std::shared_ptr(
        maybe_source.value()));

    return LDStatus_Success();
}
