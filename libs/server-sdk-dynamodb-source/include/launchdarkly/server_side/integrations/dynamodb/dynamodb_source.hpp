/** @file dynamodb_source.hpp
 * @brief Server-Side DynamoDB Source
 */

#pragma once

#include <launchdarkly/server_side/integrations/data_reader/iserialized_data_reader.hpp>
#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace Aws::DynamoDB {
class DynamoDBClient;
}

namespace launchdarkly::server_side::integrations {

/**
 * @brief DynamoDBDataSource represents a data source for the Server-Side SDK
 * backed by Amazon DynamoDB. It is meant to be used in place of the standard
 * LaunchDarkly Streaming or Polling data sources.
 *
 * Call DynamoDBDataSource::Create to obtain a new instance. This instance can
 * be passed into the SDK's DataSystem configuration via the LazyLoad builder.
 *
 * The DynamoDB table must already exist and follow the LaunchDarkly schema:
 * a String partition key named `namespace` and a String sort key named `key`.
 * The LaunchDarkly Relay Proxy populates the table with this schema; this
 * class only reads from it.
 *
 * This implementation is backed by the AWS SDK for C++.
 */
class DynamoDBDataSource final : public ISerializedDataReader {
   public:
    /**
     * @brief Creates a new DynamoDBDataSource, or returns an error if
     * construction failed.
     *
     * @param table_name Name of the DynamoDB table to read from. The table
     * must already exist; this class does not create it.
     *
     * @param prefix Optional namespace prefix. When non-empty, the source
     * reads rows whose partition key is `<prefix>:features`,
     * `<prefix>:segments`, etc. This allows multiple LaunchDarkly
     * environments to share a single table.
     *
     * @param options Optional AWS DynamoDB client configuration. See
     * @ref DynamoDBClientOptions. When defaulted, the AWS SDK resolves
     * region, endpoint, and credentials from the standard provider chain
     * (environment variables, shared config files, instance metadata).
     *
     * @return A DynamoDBDataSource, or an error if construction failed.
     */
    static tl::expected<std::unique_ptr<DynamoDBDataSource>, std::string>
    Create(std::string table_name,
           std::string prefix,
           DynamoDBClientOptions options = {});

    [[nodiscard]] GetResult Get(ISerializedItemKind const& kind,
                                std::string const& itemKey) const override;
    [[nodiscard]] AllResult All(ISerializedItemKind const& kind) const override;
    [[nodiscard]] std::string const& Identity() const override;
    [[nodiscard]] bool Initialized() const override;

    ~DynamoDBDataSource() override;

   private:
    DynamoDBDataSource(std::shared_ptr<Aws::DynamoDB::DynamoDBClient> client,
                       std::string table_name,
                       std::string prefix);

    std::shared_ptr<Aws::DynamoDB::DynamoDBClient> client_;
    std::string const table_name_;
    std::string const prefix_;
    std::string const inited_namespace_;
};

}  // namespace launchdarkly::server_side::integrations
