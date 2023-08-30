// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/server_side/bindings/c/sdk.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/data_source_status.hpp>

#include <boost/core/ignore_unused.hpp>
#include <cstring>

using namespace launchdarkly::server_side;
using namespace launchdarkly;

struct Detail;

#define TO_SDK(ptr) (reinterpret_cast<Client*>(ptr))
#define FROM_SDK(ptr) (reinterpret_cast<LDServerSDK>(ptr))

#define TO_CONTEXT(ptr) (reinterpret_cast<Context*>(ptr))
#define FROM_CONTEXT(ptr) (reinterpret_cast<LDContext>(ptr))

#define TO_VALUE(ptr) (reinterpret_cast<Value*>(ptr))
#define FROM_VALUE(ptr) (reinterpret_cast<LDValue>(ptr))

#define FROM_DETAIL(ptr) (reinterpret_cast<LDEvalDetail>(ptr))

#define TO_DATASOURCESTATUS(ptr) \
    (reinterpret_cast<           \
        launchdarkly::server_side::data_sources::DataSourceStatus*>(ptr))
#define FROM_DATASOURCESTATUS(ptr) (reinterpret_cast<LDDataSourceStatus>(ptr))

#define TO_DATASOURCESTATUS_ERRORINFO(ptr)                      \
    (reinterpret_cast<launchdarkly::server_side::data_sources:: \
                          DataSourceStatus::ErrorInfo*>(ptr))
#define FROM_DATASOURCESTATUS_ERRORINFO(ptr) \
    (reinterpret_cast<LDDataSourceStatus_ErrorInfo>(ptr))

#define TO_ALLFLAGS(ptr) (reinterpret_cast<AllFlagsState*>(ptr))
#define FROM_ALLFLAGS(ptr) (reinterpret_cast<LDAllFlagsState>(ptr))

/*
 * Helper to perform the common functionality of checking if the user
 * requested a detail out parameter. If so, we allocate a copy of it
 * on the heap and return it along with the result. Otherwise,
 * we let it destruct and only return the result.
 */
template <typename Callable>
inline static auto MaybeDetail(LDServerSDK sdk,
                               LDEvalDetail* out_detail,
                               Callable&& fn) {
    auto internal_detail = fn(TO_SDK(sdk));

    auto result = internal_detail.Value();

    if (!out_detail) {
        return result;
    }

    *out_detail = FROM_DETAIL(new CEvaluationDetail(internal_detail));

    return result;
}

LD_EXPORT(LDServerSDK)
LDServerSDK_New(LDServerConfig config) {
    LD_ASSERT_NOT_NULL(config);

    auto as_cfg = reinterpret_cast<Config*>(config);
    auto sdk = new Client(std::move(*as_cfg));

    LDServerConfig_Free(config);

    return FROM_SDK(sdk);
}

LD_EXPORT(char const*)
LDServerSDK_Version(void) {
    return Client::Version();
}

LD_EXPORT(bool)
LDServerSDK_Start(LDServerSDK sdk,
                  unsigned int milliseconds,
                  bool* out_succeeded) {
    LD_ASSERT_NOT_NULL(sdk);

    auto future = TO_SDK(sdk)->StartAsync();

    if (milliseconds == LD_NONBLOCKING) {
        return false;
    }

    if (future.wait_for(std::chrono::milliseconds{milliseconds}) ==
        std::future_status::ready) {
        if (out_succeeded) {
            *out_succeeded = future.get();
        }
        return true;
    }

    return false;
}

LD_EXPORT(bool) LDServerSDK_Initialized(LDServerSDK sdk) {
    LD_ASSERT_NOT_NULL(sdk);

    return TO_SDK(sdk)->Initialized();
}

LD_EXPORT(void)
LDServerSDK_TrackEvent(LDServerSDK sdk,
                       LDContext context,
                       char const* event_name) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(event_name);

    TO_SDK(sdk)->Track(*TO_CONTEXT(context), event_name);
}

LD_EXPORT(void)
LDServerSDK_TrackMetric(LDServerSDK sdk,
                        LDContext context,
                        char const* event_name,
                        double metric_value,
                        LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    Value* as_value = TO_VALUE(value);

    TO_SDK(sdk)->Track(*TO_CONTEXT(context), event_name, metric_value,
                       std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDServerSDK_TrackData(LDServerSDK sdk,
                      LDContext context,
                      char const* event_name,
                      LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    Value* as_value = TO_VALUE(value);

    TO_SDK(sdk)->Track(*TO_CONTEXT(context), event_name, std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDServerSDK_Flush(LDServerSDK sdk, unsigned int reserved) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT(reserved == LD_NONBLOCKING);
    TO_SDK(sdk)->FlushAsync();
}

LD_EXPORT(void)
LDServerSDK_Identify(LDServerSDK sdk, LDContext context) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);

    TO_SDK(sdk)->Identify(*TO_CONTEXT(context));
}

LD_EXPORT(bool)
LDServerSDK_BoolVariation(LDServerSDK sdk,
                          LDContext context,
                          char const* flag_key,
                          bool default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->BoolVariation(*TO_CONTEXT(context), flag_key,
                                      default_value);
}

LD_EXPORT(bool)
LDServerSDK_BoolVariationDetail(LDServerSDK sdk,
                                LDContext context,
                                char const* flag_key,
                                bool default_value,
                                LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->BoolVariationDetail(*TO_CONTEXT(context), flag_key,
                                           default_value);
    });
}

LD_EXPORT(char*)
LDServerSDK_StringVariation(LDServerSDK sdk,
                            LDContext context,
                            char const* flag_key,
                            char const* default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT_NOT_NULL(default_value);

    // TODO: custom allocation / free routines
    return strdup(
        TO_SDK(sdk)
            ->StringVariation(*TO_CONTEXT(context), flag_key, default_value)
            .c_str());
}

LD_EXPORT(char*)
LDServerSDK_StringVariationDetail(LDServerSDK sdk,
                                  LDContext context,
                                  char const* flag_key,
                                  char const* default_value,
                                  LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT_NOT_NULL(default_value);

    return strdup(MaybeDetail(sdk, out_detail, [&](Client* client) {
                      return client->StringVariationDetail(
                          *TO_CONTEXT(context), flag_key, default_value);
                  }).c_str());
}

LD_EXPORT(int)
LDServerSDK_IntVariation(LDServerSDK sdk,
                         LDContext context,
                         char const* flag_key,
                         int default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->IntVariation(*TO_CONTEXT(context), flag_key,
                                     default_value);
}

LD_EXPORT(int)
LDServerSDK_IntVariationDetail(LDServerSDK sdk,
                               LDContext context,
                               char const* flag_key,
                               int default_value,
                               LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->IntVariationDetail(*TO_CONTEXT(context), flag_key,
                                          default_value);
    });
}

LD_EXPORT(int)
LDServerSDK_DoubleVariation(LDServerSDK sdk,
                            LDContext context,
                            char const* flag_key,
                            double default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->DoubleVariation(*TO_CONTEXT(context), flag_key,
                                        default_value);
}

LD_EXPORT(int)
LDServerSDK_DoubleVariationDetail(LDServerSDK sdk,
                                  LDContext context,
                                  char const* flag_key,
                                  double default_value,
                                  LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->DoubleVariationDetail(*TO_CONTEXT(context), flag_key,
                                             default_value);
    });
}

LD_EXPORT(LDValue)
LDServerSDK_JsonVariation(LDServerSDK sdk,
                          LDContext context,
                          char const* flag_key,
                          LDValue default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT(default_value);

    Value* as_value = TO_VALUE(default_value);

    return FROM_VALUE(new Value(
        TO_SDK(sdk)->JsonVariation(*TO_CONTEXT(context), flag_key, *as_value)));
}

LD_EXPORT(LDValue)
LDServerSDK_JsonVariationDetail(LDServerSDK sdk,
                                LDContext context,
                                char const* flag_key,
                                LDValue default_value,
                                LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT(default_value);

    Value* as_value = TO_VALUE(default_value);

    return FROM_VALUE(
        new Value(MaybeDetail(sdk, out_detail, [&](Client* client) {
            return client->JsonVariationDetail(*TO_CONTEXT(context), flag_key,
                                               *as_value);
        })));
}

LD_EXPORT(LDAllFlagsState)
LDServerSDK_AllFlagsState(LDServerSDK sdk,
                          LDContext context,
                          enum LDAllFlagsState_Options options) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);

    AllFlagsState state = TO_SDK(sdk)->AllFlagsState(
        *TO_CONTEXT(context), static_cast<AllFlagsState::Options>(options));

    return FROM_ALLFLAGS(new AllFlagsState(std::move(state)));
}

LD_EXPORT(void) LDServerSDK_Free(LDServerSDK sdk) {
    delete TO_SDK(sdk);
}

LD_EXPORT(LDDataSourceStatus_State)
LDDataSourceStatus_GetState(LDDataSourceStatus status) {
    LD_ASSERT_NOT_NULL(status);
    return static_cast<enum LDDataSourceStatus_State>(
        TO_DATASOURCESTATUS(status)->State());
}

LD_EXPORT(LDDataSourceStatus_ErrorInfo)
LDDataSourceStatus_GetLastError(LDDataSourceStatus status) {
    LD_ASSERT_NOT_NULL(status);
    auto error = TO_DATASOURCESTATUS(status)->LastError();
    if (!error) {
        return nullptr;
    }
    return FROM_DATASOURCESTATUS_ERRORINFO(
        new data_sources::DataSourceStatus::ErrorInfo(
            error->Kind(), error->StatusCode(), error->Message(),
            error->Time()));
}

LD_EXPORT(time_t) LDDataSourceStatus_StateSince(LDDataSourceStatus status) {
    LD_ASSERT_NOT_NULL(status);

    return std::chrono::duration_cast<std::chrono::seconds>(
               TO_DATASOURCESTATUS(status)->StateSince().time_since_epoch())
        .count();
}

LD_EXPORT(LDDataSourceStatus_ErrorKind)
LDDataSourceStatus_ErrorInfo_GetKind(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    return static_cast<enum LDDataSourceStatus_ErrorKind>(
        TO_DATASOURCESTATUS_ERRORINFO(info)->Kind());
}

LD_EXPORT(uint64_t)
LDDataSourceStatus_ErrorInfo_StatusCode(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    return TO_DATASOURCESTATUS_ERRORINFO(info)->StatusCode();
}

LD_EXPORT(char const*)
LDDataSourceStatus_ErrorInfo_Message(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    return TO_DATASOURCESTATUS_ERRORINFO(info)->Message().c_str();
}

LD_EXPORT(time_t)
LDDataSourceStatus_ErrorInfo_Time(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    return std::chrono::duration_cast<std::chrono::seconds>(
               TO_DATASOURCESTATUS_ERRORINFO(info)->Time().time_since_epoch())
        .count();
}

LD_EXPORT(void)
LDDataSourceStatusListener_Init(LDDataSourceStatusListener listener) {
    listener.StatusChanged = nullptr;
    listener.UserData = nullptr;
}

LD_EXPORT(LDListenerConnection)
LDServerSDK_DataSourceStatus_OnStatusChange(
    LDServerSDK sdk,
    struct LDDataSourceStatusListener listener) {
    LD_ASSERT_NOT_NULL(sdk);

    if (listener.StatusChanged) {
        auto connection =
            TO_SDK(sdk)->DataSourceStatus().OnDataSourceStatusChange(
                [listener](data_sources::DataSourceStatus status) {
                    listener.StatusChanged(FROM_DATASOURCESTATUS(&status),
                                           listener.UserData);
                });

        return reinterpret_cast<LDListenerConnection>(connection.release());
    }

    return nullptr;
}

LD_EXPORT(LDDataSourceStatus)
LDServerSDK_DataSourceStatus_Status(LDServerSDK sdk) {
    LD_ASSERT_NOT_NULL(sdk);

    return FROM_DATASOURCESTATUS(new data_sources::DataSourceStatus(
        TO_SDK(sdk)->DataSourceStatus().Status()));
}

LD_EXPORT(void) LDDataSourceStatus_Free(LDDataSourceStatus status) {
    delete TO_DATASOURCESTATUS(status);
}

LD_EXPORT(void)
LDDataSourceStatus_ErrorInfo_Free(LDDataSourceStatus_ErrorInfo info) {
    delete TO_DATASOURCESTATUS_ERRORINFO(info);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
