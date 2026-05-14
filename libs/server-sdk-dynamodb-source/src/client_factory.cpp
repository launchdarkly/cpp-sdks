#include "client_factory.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>

namespace launchdarkly::server_side::integrations::detail {

namespace {

bool HasExplicitCredentials(DynamoDBClientOptions const& options) {
    return options.aws_access_key_id.has_value() ||
           options.aws_secret_access_key.has_value() ||
           options.aws_session_token.has_value();
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

std::shared_ptr<Aws::DynamoDB::DynamoDBClient> BuildDynamoDBClient(
    DynamoDBClientOptions const& options) {
    auto const config = BuildConfig(options);

    if (HasExplicitCredentials(options)) {
        Aws::Auth::AWSCredentials credentials{
            options.aws_access_key_id.value_or(""),
            options.aws_secret_access_key.value_or(""),
            options.aws_session_token.value_or("")};
        return std::make_shared<Aws::DynamoDB::DynamoDBClient>(credentials,
                                                               config);
    }

    return std::make_shared<Aws::DynamoDB::DynamoDBClient>(config);
}

}  // namespace launchdarkly::server_side::integrations::detail
