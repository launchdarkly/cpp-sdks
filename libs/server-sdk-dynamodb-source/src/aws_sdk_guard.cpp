#include "aws_sdk_guard.hpp"

namespace launchdarkly::server_side::integrations::detail {

void AwsSdkGuard::Ensure() {
    static AwsSdkGuard instance;
    (void)instance;
}

AwsSdkGuard::AwsSdkGuard() {
    Aws::InitAPI(options_);
}

AwsSdkGuard::~AwsSdkGuard() {
    Aws::ShutdownAPI(options_);
}

}  // namespace launchdarkly::server_side::integrations::detail
