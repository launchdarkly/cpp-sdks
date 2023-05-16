// NOLINTBEGIN modernize-use-using

#pragma once

#include <launchdarkly/bindings/c/config/config.h>
#include <launchdarkly/bindings/c/export.h>
#include <launchdarkly/bindings/c/status.h>

#ifdef __cplusplus
extern "C" {  // only need to export C interface if
// used by C++ source code
#endif

typedef struct _LDClientConfigBuilder* LDClientConfigBuilder;

/**
 * Constructs a client-side config builder.
 */
LD_EXPORT(LDClientConfigBuilder) LDClientConfigBuilder_New(char const* sdk_key);

/**
 * Sets a custom URL for the polling service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_PollingBaseURL(LDClientConfigBuilder b,
                                                      char const* url);
/**
 * Sets a custom URL for the streaming service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_StreamingBaseURL(LDClientConfigBuilder b,
                                                        char const* url);
/**
 * Sets a custom URL for the events service.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_EventsBaseURL(LDClientConfigBuilder b,
                                                     char const* url);
/**
 * Sets a custom URL for a Relay Proxy instance. The streaming,
 * polling, and events URLs are set automatically.
 * @param b Client config builder. Must not be NULL.
 * @param url Target URL. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_ServiceEndpoints_RelayProxy(LDClientConfigBuilder b,
                                                  char const* url);

/**
 * Sets an identifier for the application.
 * @param app_id Non-empty string. Must be <= 64 chars. Must be alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Identifier(LDClientConfigBuilder b,
                                         char const* app_id);

/**
 * Sets a version for the application.
 * @param app_version Non-empty string. Must be <= 64 chars. Must be
 * alphanumeric,
 * '-', '.', or '_'. Must not be NULL.
 */
LD_EXPORT(void)
LDClientConfigBuilder_AppInfo_Version(LDClientConfigBuilder b,
                                      char const* app_version);

/**
 * Creates an LDClientConfig. The LDClientConfigBuilder is consumed.
 * On success, the config will be stored in out_config; otherwise,
 * out_config will be set to NULL and the returned LDStatus will indicate
 * the error.
 * @param builder Builder to consume. Must not be NULL.
 * @param out_config Pointer to where the built config will be
 * stored. Must not be NULL.
 * @return Error status on failure.
 */
LD_EXPORT(LDStatus)
LDClientConfigBuilder_Build(LDClientConfigBuilder builder,
                            LDClientConfig* out_config);

/**
 * Frees the builder; only necessary if not calling Build.
 * @param builder Builder to free.
 */
LD_EXPORT(void)
LDClientConfigBuilder_Free(LDClientConfigBuilder builder);

#ifdef __cplusplus
}
#endif

// NOLINTEND modernize-use-using
