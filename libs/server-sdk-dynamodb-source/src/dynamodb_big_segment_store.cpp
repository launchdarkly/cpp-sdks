#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_big_segment_store.hpp>

#include "aws_sdk_guard.hpp"
#include "client_factory.hpp"
#include "dynamodb_attributes.hpp"
#include "prefix.hpp"

#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/GetItemRequest.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <utility>

namespace launchdarkly::server_side::integrations {

namespace {

using detail::kBigSegmentsExcludedAttribute;
using detail::kBigSegmentsIncludedAttribute;
using detail::kBigSegmentsMetadataNamespace;
using detail::kBigSegmentsSyncTimeAttribute;
using detail::kBigSegmentsUserNamespace;
using detail::kPartitionKey;
using detail::kSortKey;
using detail::PrefixedNamespace;

}  // namespace

tl::expected<std::unique_ptr<DynamoDBBigSegmentStore>, std::string>
DynamoDBBigSegmentStore::Create(std::string table_name,
                                std::string prefix,
                                DynamoDBClientOptions options) {
    try {
        detail::AwsSdkGuard::Ensure();
        auto maybe_client = detail::BuildDynamoDBClient(options);
        if (!maybe_client) {
            return tl::make_unexpected(std::move(maybe_client.error()));
        }
        return std::unique_ptr<DynamoDBBigSegmentStore>(
            new DynamoDBBigSegmentStore(std::move(*maybe_client),
                                        std::move(table_name),
                                        std::move(prefix)));
    } catch (std::exception const& e) {
        return tl::make_unexpected(e.what());
    }
}

DynamoDBBigSegmentStore::DynamoDBBigSegmentStore(
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client,
    std::string table_name,
    std::string prefix)
    : client_(std::move(client)),
      table_name_(std::move(table_name)),
      prefix_(std::move(prefix)),
      user_namespace_(PrefixedNamespace(prefix_, kBigSegmentsUserNamespace)),
      metadata_namespace_(
          PrefixedNamespace(prefix_, kBigSegmentsMetadataNamespace)) {}

DynamoDBBigSegmentStore::~DynamoDBBigSegmentStore() = default;

IBigSegmentStore::GetMembershipResult DynamoDBBigSegmentStore::GetMembership(
    std::string const& context_hash) const {
    Aws::DynamoDB::Model::GetItemRequest request;
    request.SetTableName(table_name_);
    request.SetConsistentRead(true);
    request.AddKey(kPartitionKey,
                   Aws::DynamoDB::Model::AttributeValue{user_namespace_});
    request.AddKey(kSortKey,
                   Aws::DynamoDB::Model::AttributeValue{context_hash});

    auto outcome = client_->GetItem(request);
    if (!outcome.IsSuccess()) {
        return tl::make_unexpected(outcome.GetError().GetMessage());
    }

    auto const& item = outcome.GetResult().GetItem();
    if (item.empty()) {
        return std::nullopt;
    }

    std::vector<std::string> included;
    std::vector<std::string> excluded;

    // GetSS() silently returns an empty vector if the attribute is not
    // actually a String Set, so check the type explicitly before reading.
    if (auto const it = item.find(kBigSegmentsIncludedAttribute);
        it != item.end()) {
        if (it->second.GetType() !=
            Aws::DynamoDB::Model::ValueType::STRING_SET) {
            return tl::make_unexpected(
                std::string("DynamoDB Big Segments '") +
                kBigSegmentsIncludedAttribute +
                "' is not of type STRING_SET");
        }
        for (auto const& ref : it->second.GetSS()) {
            included.emplace_back(ref);
        }
    }
    if (auto const it = item.find(kBigSegmentsExcludedAttribute);
        it != item.end()) {
        if (it->second.GetType() !=
            Aws::DynamoDB::Model::ValueType::STRING_SET) {
            return tl::make_unexpected(
                std::string("DynamoDB Big Segments '") +
                kBigSegmentsExcludedAttribute +
                "' is not of type STRING_SET");
        }
        for (auto const& ref : it->second.GetSS()) {
            excluded.emplace_back(ref);
        }
    }

    return Membership::FromSegmentRefs(included, excluded);
}

IBigSegmentStore::GetMetadataResult DynamoDBBigSegmentStore::GetMetadata()
    const {
    Aws::DynamoDB::Model::GetItemRequest request;
    request.SetTableName(table_name_);
    request.SetConsistentRead(true);
    request.AddKey(kPartitionKey,
                   Aws::DynamoDB::Model::AttributeValue{metadata_namespace_});
    request.AddKey(kSortKey,
                   Aws::DynamoDB::Model::AttributeValue{metadata_namespace_});

    auto outcome = client_->GetItem(request);
    if (!outcome.IsSuccess()) {
        return tl::make_unexpected(outcome.GetError().GetMessage());
    }

    auto const& item = outcome.GetResult().GetItem();
    if (item.empty()) {
        return std::nullopt;
    }

    auto const it = item.find(kBigSegmentsSyncTimeAttribute);
    if (it == item.end()) {
        return tl::make_unexpected(
            "DynamoDB Big Segments metadata row missing 'synchronizedOn'");
    }

    auto const& raw = it->second.GetN();
    if (raw.empty()) {
        return tl::make_unexpected(
            "DynamoDB Big Segments 'synchronizedOn' is empty or not type N");
    }

    errno = 0;
    char* end = nullptr;
    long long const parsed = std::strtoll(raw.c_str(), &end, 10);
    if (errno != 0 || end == raw.c_str() || *end != '\0') {
        return tl::make_unexpected(
            "DynamoDB Big Segments 'synchronizedOn' is not a valid integer");
    }

    return StoreMetadata{std::chrono::milliseconds{parsed}};
}

}  // namespace launchdarkly::server_side::integrations
