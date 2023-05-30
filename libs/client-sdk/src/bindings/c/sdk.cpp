// NOLINTBEGIN cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTBEGIN OCInconsistentNamingInspection

#include <launchdarkly/bindings/c/array_builder.h>
#include <launchdarkly/client_side/bindings/c/sdk.h>
#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <boost/core/ignore_unused.hpp>
#include <cstring>

using namespace launchdarkly::client_side;
using namespace launchdarkly;

struct Detail;

#define TO_SDK(ptr) (reinterpret_cast<Client*>(ptr))
#define FROM_SDK(ptr) (reinterpret_cast<LDClientSDK>(ptr))

#define FROM_DETAIL(ptr) (reinterpret_cast<LDEvalDetail>(ptr))

#define TO_DATASOURCESTATUS(ptr) \
    (reinterpret_cast<           \
        launchdarkly::client_side::data_sources::DataSourceStatus*>(ptr))
#define FROM_DATASOURCESTATUS(ptr) (reinterpret_cast<LDDataSourceStatus>(ptr))

#define TO_DATASOURCESTATUS_ERRORINFO(ptr)                      \
    (reinterpret_cast<launchdarkly::client_side::data_sources:: \
                          DataSourceStatus::ErrorInfo*>(ptr))
#define FROM_DATASOURCESTATUS_ERRORINFO(ptr) \
    (reinterpret_cast<LDDataSourceStatus_ErrorInfo>(ptr))

/*
 * Helper to perform the common functionality of checking if the user
 * requested a detail out parameter. If so, we allocate a copy of it
 * on the heap and return it along with the result. Otherwise,
 * we let it destruct and only return the result.
 */
template <typename Callable>
inline static auto MaybeDetail(LDClientSDK sdk,
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

LD_EXPORT(LDClientSDK)
LDClientSDK_New(LDClientConfig config, LDContext context) {
    LD_ASSERT_NOT_NULL(config);
    LD_ASSERT_NOT_NULL(context);

    auto as_cfg = reinterpret_cast<Config*>(config);
    auto as_ctx = reinterpret_cast<Context*>(context);

    auto sdk = new Client(std::move(*as_cfg), std::move(*as_ctx));

    LDClientConfig_Free(config);
    LDContext_Free(context);

    return FROM_SDK(sdk);
}

LD_EXPORT(bool)
LDClientSDK_Start(LDClientSDK sdk,
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

LD_EXPORT(bool) LDClientSDK_Initialized(LDClientSDK sdk) {
    LD_ASSERT_NOT_NULL(sdk);

    return TO_SDK(sdk)->Initialized();
}

LD_EXPORT(void)
LDClientSDK_TrackEvent(LDClientSDK sdk, char const* event_name) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);

    TO_SDK(sdk)->Track(event_name);
}

LD_EXPORT(void)
LDClientSDK_TrackMetric(LDClientSDK sdk,
                        char const* event_name,
                        double metric_value,
                        LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    auto as_value = reinterpret_cast<Value*>(value);

    TO_SDK(sdk)->Track(event_name, metric_value, std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDClientSDK_TrackData(LDClientSDK sdk, char const* event_name, LDValue value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(event_name);
    LD_ASSERT_NOT_NULL(value);

    auto as_value = reinterpret_cast<Value*>(value);

    TO_SDK(sdk)->Track(event_name, std::move(*as_value));

    LDValue_Free(value);
}

LD_EXPORT(void)
LDClientSDK_Flush(LDClientSDK sdk, unsigned int reserved) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT(reserved == LD_NONBLOCKING);
    TO_SDK(sdk)->FlushAsync();
}

LD_EXPORT(bool)
LDClientSDK_Identify(LDClientSDK sdk,
                     LDContext context,
                     unsigned int milliseconds,
                     bool* out_succeeded) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(context);

    auto as_ctx = reinterpret_cast<Context*>(context);
    auto future = TO_SDK(sdk)->IdentifyAsync(std::move(*as_ctx));

    LDContext_Free(context);

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

LD_EXPORT(bool)
LDClientSDK_BoolVariation(LDClientSDK sdk,
                          char const* flag_key,
                          bool default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->BoolVariation(flag_key, default_value);
}

LD_EXPORT(bool)
LDClientSDK_BoolVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                bool default_value,
                                LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->BoolVariationDetail(flag_key, default_value);
    });
}

LD_EXPORT(char*)
LDClientSDK_StringVariation(LDClientSDK sdk,
                            char const* flag_key,
                            char const* default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT_NOT_NULL(default_value);

    // TODO: custom allocation / free routines
    return strdup(
        TO_SDK(sdk)->StringVariation(flag_key, default_value).c_str());
}

LD_EXPORT(char*)
LDClientSDK_StringVariationDetail(LDClientSDK sdk,
                                  char const* flag_key,
                                  char const* default_value,
                                  LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT_NOT_NULL(default_value);

    return strdup(MaybeDetail(sdk, out_detail, [&](Client* client) {
                      return client->StringVariationDetail(flag_key,
                                                           default_value);
                  }).c_str());
}

LD_EXPORT(int)
LDClientSDK_IntVariation(LDClientSDK sdk,
                         char const* flag_key,
                         int default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->IntVariation(flag_key, default_value);
}

LD_EXPORT(int)
LDClientSDK_IntVariationDetail(LDClientSDK sdk,
                               char const* flag_key,
                               int default_value,
                               LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->IntVariationDetail(flag_key, default_value);
    });
}

LD_EXPORT(int)
LDClientSDK_DoubleVariation(LDClientSDK sdk,
                            char const* flag_key,
                            double default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return TO_SDK(sdk)->DoubleVariation(flag_key, default_value);
}

LD_EXPORT(int)
LDClientSDK_DoubleVariationDetail(LDClientSDK sdk,
                                  char const* flag_key,
                                  double default_value,
                                  LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    return MaybeDetail(sdk, out_detail, [&](Client* client) {
        return client->DoubleVariationDetail(flag_key, default_value);
    });
}

LD_EXPORT(LDValue)
LDClientSDK_JsonVariation(LDClientSDK sdk,
                          char const* flag_key,
                          LDValue default_value) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT(default_value);

    auto as_value = reinterpret_cast<Value*>(default_value);

    return reinterpret_cast<LDValue>(
        new Value(TO_SDK(sdk)->JsonVariation(flag_key, *as_value)));
}

LD_EXPORT(LDValue)
LDClientSDK_JsonVariationDetail(LDClientSDK sdk,
                                char const* flag_key,
                                LDValue default_value,
                                LDEvalDetail* out_detail) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);
    LD_ASSERT(default_value);

    auto as_value = reinterpret_cast<Value*>(default_value);

    return reinterpret_cast<LDValue>(
        new Value(MaybeDetail(sdk, out_detail, [&](Client* client) {
            return client->JsonVariationDetail(flag_key, *as_value);
        })));
}

LD_EXPORT(void) LDClientSDK_Free(LDClientSDK sdk) {
    delete TO_SDK(sdk);
}

LD_EXPORT(LDListenerConnection)
LDClientSDK_FlagNotifier_OnFlagChange(LDClientSDK sdk,
                                      char const* flag_key,
                                      struct LDFlagListener listener) {
    LD_ASSERT_NOT_NULL(sdk);
    LD_ASSERT_NOT_NULL(flag_key);

    if (listener.FlagChanged) {
        auto connection = TO_SDK(sdk)->FlagNotifier().OnFlagChange(
            flag_key,
            [listener](std::shared_ptr<launchdarkly::client_side::flag_manager::
                                           FlagValueChangeEvent> event) {
                listener.FlagChanged(
                    event->FlagName().c_str(),
                    reinterpret_cast<LDValue>(
                        const_cast<Value*>(&event->NewValue())),
                    reinterpret_cast<LDValue>(
                        const_cast<Value*>(&event->OldValue())),
                    event->Deleted(), listener.UserData);
            });

        return reinterpret_cast<LDListenerConnection>(connection.release());
    }

    return nullptr;
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

LD_EXPORT(void) LDFlagListener_Init(struct LDFlagListener listener) {
    listener.FlagChanged = nullptr;
    listener.UserData = nullptr;
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

    TO_DATASOURCESTATUS_ERRORINFO(info)->StatusCode();
}

LD_EXPORT(char const*)
LDDataSourceStatus_ErrorInfo_Message(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    TO_DATASOURCESTATUS_ERRORINFO(info)->Message().c_str();
}

LD_EXPORT(time_t)
LDDataSourceStatus_ErrorInfo_Time(LDDataSourceStatus_ErrorInfo info) {
    LD_ASSERT_NOT_NULL(info);

    std::chrono::duration_cast<std::chrono::seconds>(
        TO_DATASOURCESTATUS_ERRORINFO(info)->Time().time_since_epoch())
        .count();
}

LD_EXPORT(void)
LDDataSourceStatusListener_Init(LDDataSourceStatusListener listener) {
    listener.StatusChanged = nullptr;
    listener.UserData = nullptr;
}

LD_EXPORT(LDListenerConnection)
LDClientSDK_DataSourceStatus_OnStatusChange(
    LDClientSDK sdk,
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
LDClientSDK_DataSourceStatus_Status(LDClientSDK sdk) {
    LD_ASSERT_NOT_NULL(sdk);

    return FROM_DATASOURCESTATUS(new data_sources::DataSourceStatus(
        TO_SDK(sdk)->DataSourceStatus().Status()));
}

LD_EXPORT(void) LDDataSourceStatus_Free(LDDataSourceStatus status) {
    delete TO_DATASOURCESTATUS(status);
}

LD_EXPORT(void) LDDataSourceStatus_Free(LDDataSourceStatus_ErrorInfo info) {
    delete TO_DATASOURCESTATUS_ERRORINFO(info);
}

// NOLINTEND cppcoreguidelines-pro-type-reinterpret-cast
// NOLINTEND OCInconsistentNamingInspection
