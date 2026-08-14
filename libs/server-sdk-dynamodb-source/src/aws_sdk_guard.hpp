#pragma once

#include <aws/core/Aws.h>

namespace launchdarkly::server_side::integrations::detail {

// AwsSdkGuard owns the process-wide Aws::InitAPI / Aws::ShutdownAPI lifecycle
// for this library. Multiple DynamoDB-backed integrations within the same
// process share the single static instance; the API is initialized lazily on
// first use and torn down during normal program termination via C++ static
// destruction.
//
// Static-destruction ordering caveat: if a caller stashes a raw AWS SDK
// pointer in their own static and that static is destroyed AFTER this guard,
// AWS SDK calls during that destructor will be undefined. The standard usage
// pattern (holding the data source / store via a unique_ptr or shared_ptr in
// regular program scope, not in another static) is unaffected because those
// smart pointers destruct before the guard.
class AwsSdkGuard {
   public:
    // Idempotent. First call constructs the singleton, which runs
    // Aws::InitAPI in its constructor. Subsequent calls are no-ops. Safe to
    // call from any thread.
    static void Ensure();

    AwsSdkGuard(AwsSdkGuard const&) = delete;
    AwsSdkGuard(AwsSdkGuard&&) = delete;
    AwsSdkGuard& operator=(AwsSdkGuard const&) = delete;
    AwsSdkGuard& operator=(AwsSdkGuard&&) = delete;

   private:
    AwsSdkGuard();
    ~AwsSdkGuard();

    Aws::SDKOptions options_;
};

}  // namespace launchdarkly::server_side::integrations::detail
