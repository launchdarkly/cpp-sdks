#include "client_factory.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>

namespace launchdarkly::server_side::integrations::detail {

namespace {

// Verifies that the credential fields in `options` form a valid combination.
// Returns an empty optional on success, or an error string describing what's
// wrong. The valid combinations are:
//
//   - none of the three set (fall through to AWS default credential chain)
//   - access_key_id + secret_access_key (long-lived IAM keys)
//   - access_key_id + secret_access_key + session_token (STS temporary creds)
//
// All other partial combinations would build a misconfigured AWS client that
// fails opaquely at request time; catching them here surfaces the
// misconfiguration up front.
std::optional<std::string> ValidateCredentials(
    DynamoDBClientOptions const& options) {
    bool const has_key = options.aws_access_key_id.has_value();
    bool const has_secret = options.aws_secret_access_key.has_value();
    bool const has_token = options.aws_session_token.has_value();

    if (has_key != has_secret) {
        return "aws_access_key_id and aws_secret_access_key must both be set "
               "or both unset";
    }
    if (has_token && !has_key) {
        return "aws_session_token requires aws_access_key_id and "
               "aws_secret_access_key";
    }
    return std::nullopt;
}

Aws::Client::ClientConfiguration BuildConfig(
    DynamoDBClientOptions const& options) {
    Aws::Client::ClientConfiguration config;

    if (options.region) {
        config.region = *options.region;
    }

    if (options.endpoint) {
        config.endpointOverride = *options.endpoint;
        // Use HTTP if the endpoint starts with "http://"; otherwise default
        // to HTTPS. Endpoint overrides are commonly DynamoDB Local or
        // LocalStack on plain HTTP for development.
        std::string const& ep = *options.endpoint;
        if (ep.rfind("http://", 0) == 0) {
            config.scheme = Aws::Http::Scheme::HTTP;
            config.verifySSL = false;
        }
    }

    return config;
}

}  // namespace

tl::expected<std::shared_ptr<Aws::DynamoDB::DynamoDBClient>, std::string>
BuildDynamoDBClient(DynamoDBClientOptions const& options) {
    if (auto err = ValidateCredentials(options)) {
        return tl::make_unexpected(std::move(*err));
    }

    auto const config = BuildConfig(options);

    if (options.aws_access_key_id) {
        Aws::Auth::AWSCredentials credentials{
            *options.aws_access_key_id, *options.aws_secret_access_key,
            options.aws_session_token.value_or("")};
        return std::make_shared<Aws::DynamoDB::DynamoDBClient>(credentials,
                                                               config);
    }

    return std::make_shared<Aws::DynamoDB::DynamoDBClient>(config);
}

}  // namespace launchdarkly::server_side::integrations::detail
