/** @file dynamodb_big_segment_store.hpp
 * @brief Server-Side DynamoDB Big Segments Store
 */

#pragma once

#include <launchdarkly/server_side/integrations/big_segments/ibig_segment_store.hpp>
#include <launchdarkly/server_side/integrations/dynamodb/options.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <string>

namespace Aws::DynamoDB {
class DynamoDBClient;
}

namespace launchdarkly::server_side::integrations {

/**
 * @brief DynamoDBBigSegmentStore is a Big Segments persistent store backed by
 * Amazon DynamoDB.
 *
 * Call DynamoDBBigSegmentStore::Create to obtain a new instance, then pass it
 * to the SDK via the Big Segments config builder.
 *
 * The DynamoDB table must already exist and follow the LaunchDarkly schema:
 * a String partition key named `namespace` and a String sort key named `key`.
 * The same table can be shared with @ref DynamoDBDataSource — Big Segments
 * rows occupy their own partition-key values and do not conflict with
 * flag/segment rows. The LaunchDarkly Relay Proxy is responsible for
 * populating Big Segments data in this table; this class only reads from it.
 *
 * This implementation is backed by the AWS SDK for C++.
 */
class DynamoDBBigSegmentStore final : public IBigSegmentStore {
   public:
    /**
     * @brief Creates a new DynamoDBBigSegmentStore, or returns an error if
     * construction failed.
     *
     * @param table_name Name of the DynamoDB table to read from. The table
     * must already exist; this class does not create it.
     *
     * @param prefix Optional namespace prefix. When non-empty, Big Segments
     * rows live under partition keys `<prefix>:big_segments_user` and
     * `<prefix>:big_segments_metadata`. This allows multiple LaunchDarkly
     * environments to share a single table.
     *
     * @param options Optional AWS DynamoDB client configuration. See
     * @ref DynamoDBClientOptions. When defaulted, the AWS SDK resolves
     * region, endpoint, and credentials from the standard provider chain
     * (environment variables, shared config files, instance metadata).
     *
     * @return A DynamoDBBigSegmentStore, or an error if construction failed.
     */
    static tl::expected<std::unique_ptr<DynamoDBBigSegmentStore>, std::string>
    Create(std::string table_name,
           std::string prefix,
           DynamoDBClientOptions options = {});

    [[nodiscard]] GetMembershipResult GetMembership(
        std::string const& context_hash) const override;
    [[nodiscard]] GetMetadataResult GetMetadata() const override;

    ~DynamoDBBigSegmentStore() override;

   private:
    DynamoDBBigSegmentStore(
        std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client,
        std::string table_name,
        std::string prefix);

    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client_;
    std::string const table_name_;
    std::string const prefix_;
    std::string const user_namespace_;
    std::string const metadata_namespace_;
};

}  // namespace launchdarkly::server_side::integrations
