#include <launchdarkly/bindings/c/listener_connection.h>
#include <launchdarkly/detail/c_binding_helpers.hpp>

#include <launchdarkly/connection.hpp>

#include <memory>

#define TO_LC(ptr) (reinterpret_cast<launchdarkly::IConnection*>(ptr))

LD_EXPORT(void)
LDListenerConnection_Disconnect(LDListenerConnection connection) {
    LD_ASSERT_NOT_NULL(connection);
    TO_LC(connection)->Disconnect();
}

LD_EXPORT(void) LDListenerConnection_Free(LDListenerConnection connection) {
    delete TO_LC(connection);
}
