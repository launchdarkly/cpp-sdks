#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_source.hpp>

#include "aws_sdk_guard.hpp"
#include "client_factory.hpp"
#include "dynamodb_attributes.hpp"
#include "prefix.hpp"

#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/QueryRequest.h>

#include <exception>
#include <utility>

namespace launchdarkly::server_side::integrations {

namespace {

using detail::kInitedNamespace;
using detail::kItemAttribute;
using detail::kPartitionKey;
using detail::kSortKey;
using detail::PrefixedNamespace;

}  // namespace

tl::expected<std::unique_ptr<DynamoDBDataSource>, std::string>
DynamoDBDataSource::Create(std::string table_name,
                           std::string prefix,
                           DynamoDBClientOptions options) {
    try {
        detail::AwsSdkGuard::Ensure();
        auto maybe_client = detail::BuildDynamoDBClient(options);
        if (!maybe_client) {
            return tl::make_unexpected(std::move(maybe_client.error()));
        }
        return std::unique_ptr<DynamoDBDataSource>(
            new DynamoDBDataSource(std::move(*maybe_client),
                                   std::move(table_name), std::move(prefix)));
    } catch (std::exception const& e) {
        return tl::make_unexpected(e.what());
    }
}

DynamoDBDataSource::DynamoDBDataSource(
    std::shared_ptr<Aws::DynamoDB::DynamoDBClient> client,
    std::string table_name,
    std::string prefix)
    : client_(std::move(client)),
      table_name_(std::move(table_name)),
      prefix_(std::move(prefix)),
      inited_namespace_(PrefixedNamespace(prefix_, kInitedNamespace)) {}

DynamoDBDataSource::~DynamoDBDataSource() = default;

ISerializedDataReader::GetResult DynamoDBDataSource::Get(
    ISerializedItemKind const& kind,
    std::string const& itemKey) const {
    Aws::DynamoDB::Model::GetItemRequest request;
    request.SetTableName(table_name_);
    request.SetConsistentRead(true);
    request.AddKey(kPartitionKey,
                   Aws::DynamoDB::Model::AttributeValue{
                       PrefixedNamespace(prefix_, kind.Namespace())});
    request.AddKey(kSortKey, Aws::DynamoDB::Model::AttributeValue{itemKey});

    auto outcome = client_->GetItem(request);
    if (!outcome.IsSuccess()) {
        return tl::make_unexpected(Error{outcome.GetError().GetMessage()});
    }

    auto const& item = outcome.GetResult().GetItem();
    if (item.empty()) {
        return std::nullopt;
    }

    auto const it = item.find(kItemAttribute);
    if (it == item.end()) {
        return tl::make_unexpected(
            Error{"DynamoDB row missing expected 'item' attribute"});
    }

    return SerializedItemDescriptor::Present(0, it->second.GetS());
}

ISerializedDataReader::AllResult DynamoDBDataSource::All(
    ISerializedItemKind const& kind) const {
    AllResult::value_type items;

    Aws::DynamoDB::Model::QueryRequest request;
    request.SetTableName(table_name_);
    request.SetConsistentRead(true);
    request.SetKeyConditionExpression("#ns = :ns");
    request.AddExpressionAttributeNames("#ns", kPartitionKey);
    request.AddExpressionAttributeValues(
        ":ns", Aws::DynamoDB::Model::AttributeValue{
                   PrefixedNamespace(prefix_, kind.Namespace())});

    while (true) {
        auto outcome = client_->Query(request);
        if (!outcome.IsSuccess()) {
            return tl::make_unexpected(Error{outcome.GetError().GetMessage()});
        }

        auto const& result = outcome.GetResult();
        for (auto const& row : result.GetItems()) {
            auto const key_it = row.find(kSortKey);
            if (key_it == row.end()) {
                continue;
            }
            auto const item_it = row.find(kItemAttribute);
            if (item_it == row.end()) {
                return tl::make_unexpected(
                    Error{"DynamoDB row missing expected 'item' attribute"});
            }
            items.emplace(
                key_it->second.GetS(),
                SerializedItemDescriptor::Present(0, item_it->second.GetS()));
        }

        auto const& last_key = result.GetLastEvaluatedKey();
        if (last_key.empty()) {
            break;
        }
        request.SetExclusiveStartKey(last_key);
    }

    return items;
}

std::string const& DynamoDBDataSource::Identity() const {
    static std::string const identity = "dynamodb";
    return identity;
}

bool DynamoDBDataSource::Initialized() const {
    Aws::DynamoDB::Model::GetItemRequest request;
    request.SetTableName(table_name_);
    request.SetConsistentRead(true);
    request.AddKey(kPartitionKey,
                   Aws::DynamoDB::Model::AttributeValue{inited_namespace_});
    request.AddKey(kSortKey,
                   Aws::DynamoDB::Model::AttributeValue{inited_namespace_});

    auto outcome = client_->GetItem(request);
    if (!outcome.IsSuccess()) {
        return false;
    }
    return !outcome.GetResult().GetItem().empty();
}

}  // namespace launchdarkly::server_side::integrations
