#include <launchdarkly/network/detail/http_error_messages.hpp>

namespace launchdarkly::network::detail {

std::string ErrorForStatusCode(HttpResult::StatusCode code,
                               std::string context,
                               std::optional<std::string> retry_message) {
    std::stringstream error_message;
    error_message << "HTTP error " << code
                  << (IsInvalidSdkKeyStatus(code) ? "(invalid SDK key) " : " ")
                  << "for: " << context << " - "
                  << (retry_message ? *retry_message : "giving up permanently");
    return error_message.str();
}

bool IsInvalidSdkKeyStatus(HttpResult::StatusCode code) {
    return code == 401 || code == 403;
}

}  // namespace launchdarkly::network::detail
