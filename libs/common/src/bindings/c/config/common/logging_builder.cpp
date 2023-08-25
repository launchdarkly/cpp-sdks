#include <launchdarkly/bindings/c/config/common/logging_builder.h>
#include <launchdarkly/config/shared/builders/logging_builder.hpp>

#include <launchdarkly/detail/c_binding_helpers.hpp>

#include "log_backend_wrapper.hpp"

using namespace launchdarkly::config::shared::builders;

#define TO_BASIC_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LoggingBuilder::BasicLogging*>(ptr))

#define FROM_BASIC_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LDLoggingBasicBuilder>(ptr))

#define TO_CUSTOM_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LoggingBuilder::CustomLogging*>(ptr))

#define FROM_CUSTOM_LOGGING_BUILDER(ptr) \
    (reinterpret_cast<LDLoggingCustomBuilder>(ptr))

LD_EXPORT(void) LDLogBackend_Init(struct LDLogBackend* backend) {
    backend->Enabled = [](enum LDLogLevel, void*) { return false; };
    backend->Write = [](enum LDLogLevel, char const*, void*) {};
    backend->UserData = nullptr;
}

LD_EXPORT(LDLoggingBasicBuilder) LDLoggingBasicBuilder_New() {
    return FROM_BASIC_LOGGING_BUILDER(new LoggingBuilder::BasicLogging());
}

LD_EXPORT(void) LDLoggingBasicBuilder_Free(LDLoggingBasicBuilder b) {
    delete TO_BASIC_LOGGING_BUILDER(b);
}

LD_EXPORT(void)
LDLoggingBasicBuilder_Level(LDLoggingBasicBuilder b, enum LDLogLevel level) {
    using launchdarkly::LogLevel;
    LD_ASSERT_NOT_NULL(b);

    LoggingBuilder::BasicLogging* logger = TO_BASIC_LOGGING_BUILDER(b);
    logger->Level(static_cast<LogLevel>(level));
}

void LDLoggingBasicBuilder_Tag(LDLoggingBasicBuilder b, char const* tag) {
    LD_ASSERT_NOT_NULL(b);
    LD_ASSERT_NOT_NULL(tag);

    TO_BASIC_LOGGING_BUILDER(b)->Tag(tag);
}

LD_EXPORT(LDLoggingCustomBuilder) LDLoggingCustomBuilder_New() {
    return FROM_CUSTOM_LOGGING_BUILDER(new LoggingBuilder::CustomLogging());
}

LD_EXPORT(void) LDLoggingCustomBuilder_Free(LDLoggingCustomBuilder b) {
    delete TO_CUSTOM_LOGGING_BUILDER(b);
}

LD_EXPORT(void)
LDLoggingCustomBuilder_Backend(LDLoggingCustomBuilder b, LDLogBackend backend) {
    LD_ASSERT_NOT_NULL(b);

    TO_CUSTOM_LOGGING_BUILDER(b)->Backend(
        std::make_shared<LogBackendWrapper>(backend));
}
