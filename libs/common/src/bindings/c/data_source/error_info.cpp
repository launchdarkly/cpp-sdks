#include <launchdarkly/bindings/c/data_source/error_info.h>

#include <launchdarkly/data_sources/data_source_status_base.hpp>
#include <launchdarkly/detail/c_binding_helpers.hpp>

using namespace launchdarkly::common;

#define TO_DATASOURCESTATUS_ERRORINFO(ptr) \
    (reinterpret_cast<                     \
        launchdarkly::common::data_sources::DataSourceStatusErrorInfo*>(ptr))

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
LDDataSourceStatus_ErrorInfo_Free(LDDataSourceStatus_ErrorInfo info) {
    delete TO_DATASOURCESTATUS_ERRORINFO(info);
}
