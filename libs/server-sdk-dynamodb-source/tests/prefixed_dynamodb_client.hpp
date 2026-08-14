#pragma once

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/DynamoDBErrors.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <aws/dynamodb/model/CreateTableRequest.h>
#include <aws/dynamodb/model/DeleteTableRequest.h>
#include <aws/dynamodb/model/DescribeTableRequest.h>
#include <aws/dynamodb/model/KeySchemaElement.h>
#include <aws/dynamodb/model/KeyType.h>
#include <aws/dynamodb/model/ProvisionedThroughput.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/ScalarAttributeType.h>

#include <boost/json.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

// PrefixedDynamoDBClient is a test fixture helper that writes flags and
// segments directly into a DynamoDB table using the LaunchDarkly schema
// (namespace + key), mirroring the Redis source's PrefixedClient.
class PrefixedDynamoDBClient {
   public:
    PrefixedDynamoDBClient(Aws::DynamoDB::DynamoDBClient& client,
                           std::string prefix,
                           std::string table_name)
        : client_(client),
          prefix_(std::move(prefix)),
          table_name_(std::move(table_name)) {}

    static void CreateTable(Aws::DynamoDB::DynamoDBClient& client,
                            std::string const& table_name) {
        Aws::DynamoDB::Model::CreateTableRequest request;
        request.SetTableName(table_name);

        Aws::DynamoDB::Model::KeySchemaElement partition;
        partition.SetAttributeName("namespace");
        partition.SetKeyType(Aws::DynamoDB::Model::KeyType::HASH);
        request.AddKeySchema(partition);

        Aws::DynamoDB::Model::KeySchemaElement sort;
        sort.SetAttributeName("key");
        sort.SetKeyType(Aws::DynamoDB::Model::KeyType::RANGE);
        request.AddKeySchema(sort);

        Aws::DynamoDB::Model::AttributeDefinition partition_def;
        partition_def.SetAttributeName("namespace");
        partition_def.SetAttributeType(
            Aws::DynamoDB::Model::ScalarAttributeType::S);
        request.AddAttributeDefinitions(partition_def);

        Aws::DynamoDB::Model::AttributeDefinition sort_def;
        sort_def.SetAttributeName("key");
        sort_def.SetAttributeType(Aws::DynamoDB::Model::ScalarAttributeType::S);
        request.AddAttributeDefinitions(sort_def);

        Aws::DynamoDB::Model::ProvisionedThroughput throughput;
        throughput.SetReadCapacityUnits(5);
        throughput.SetWriteCapacityUnits(5);
        request.SetProvisionedThroughput(throughput);

        auto outcome = client.CreateTable(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't create DynamoDB table " << table_name << ": "
                   << outcome.GetError().GetMessage();
        }
    }

    static void DeleteTable(Aws::DynamoDB::DynamoDBClient& client,
                            std::string const& table_name) {
        Aws::DynamoDB::Model::DeleteTableRequest request;
        request.SetTableName(table_name);
        auto outcome = client.DeleteTable(request);
        if (!outcome.IsSuccess()) {
            // Tolerate not-found so setup can call this unconditionally.
            if (outcome.GetError().GetErrorType() ==
                Aws::DynamoDB::DynamoDBErrors::RESOURCE_NOT_FOUND) {
                return;
            }
            FAIL() << "couldn't delete DynamoDB table " << table_name << ": "
                   << outcome.GetError().GetMessage();
        }
    }

    static bool TableExists(Aws::DynamoDB::DynamoDBClient& client,
                            std::string const& table_name) {
        Aws::DynamoDB::Model::DescribeTableRequest request;
        request.SetTableName(table_name);
        return client.DescribeTable(request).IsSuccess();
    }

    void Init() const {
        std::string const inited_key = Prefixed("$inited");
        PutRaw(inited_key, inited_key, /*item_attribute=*/std::nullopt);
    }

    void PutFlag(launchdarkly::data_model::Flag const& flag) const {
        PutRaw(Prefixed("features"), flag.key,
               serialize(boost::json::value_from(flag)));
    }

    void PutSegment(launchdarkly::data_model::Segment const& segment) const {
        PutRaw(Prefixed("segments"), segment.key,
               serialize(boost::json::value_from(segment)));
    }

    void PutDeletedFlag(std::string const& key, std::string const& ts) const {
        PutRaw(Prefixed("features"), key, ts);
    }

    void PutDeletedSegment(std::string const& key,
                           std::string const& ts) const {
        PutRaw(Prefixed("segments"), key, ts);
    }

    // Writes a row with only namespace + key, no `item` attribute. Used to
    // simulate corrupted/malformed rows in the table.
    void PutRowWithoutItem(std::string const& ns_suffix,
                           std::string const& key) const {
        PutRaw(Prefixed(ns_suffix), key, std::nullopt);
    }

    // Writes a row where the `item` attribute is stored as a DynamoDB Number
    // (type `N`), not a String (type `S`). DynamoDB does not enforce a type
    // schema on non-key attributes, so a non-Relay writer (manual put-item,
    // schema-migration tool, compromised writer) can produce rows with the
    // wrong attribute type. Used to verify the source surfaces this as a
    // structured error instead of silently producing an empty payload.
    void PutRowWithNumericItem(std::string const& ns_suffix,
                               std::string const& key) const {
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{
                                         Prefixed(ns_suffix)});
        request.AddItem("key", Aws::DynamoDB::Model::AttributeValue{key});

        Aws::DynamoDB::Model::AttributeValue numeric_item;
        numeric_item.SetN("12345");
        request.AddItem("item", numeric_item);

        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB item ns=" << Prefixed(ns_suffix)
                   << " key=" << key << ": " << outcome.GetError().GetMessage();
        }
    }

    void PutBigSegmentMembership(
        std::string const& context_hash,
        std::vector<std::string> const& included,
        std::vector<std::string> const& excluded) const {
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{
                                         Prefixed("big_segments_user")});
        request.AddItem("key",
                        Aws::DynamoDB::Model::AttributeValue{context_hash});
        if (!included.empty()) {
            Aws::DynamoDB::Model::AttributeValue value;
            value.SetSS(
                Aws::Vector<Aws::String>(included.begin(), included.end()));
            request.AddItem("included", value);
        }
        if (!excluded.empty()) {
            Aws::DynamoDB::Model::AttributeValue value;
            value.SetSS(
                Aws::Vector<Aws::String>(excluded.begin(), excluded.end()));
            request.AddItem("excluded", value);
        }
        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB big-segments membership: "
                   << outcome.GetError().GetMessage();
        }
    }

    // Writes a membership row where `included` is stored as a String (S)
    // instead of a String Set (SS). DynamoDB does not enforce non-key
    // attribute types, so a non-Relay writer can produce this shape; we want
    // the store to surface it as an error rather than silently producing an
    // empty membership.
    void PutMalformedBigSegmentMembership(
        std::string const& context_hash) const {
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{
                                         Prefixed("big_segments_user")});
        request.AddItem("key",
                        Aws::DynamoDB::Model::AttributeValue{context_hash});
        request.AddItem("included",
                        Aws::DynamoDB::Model::AttributeValue{"seg1.g1"});

        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put malformed DynamoDB big-segments membership: "
                   << outcome.GetError().GetMessage();
        }
    }

    void PutBigSegmentSyncTime(std::int64_t millis) const {
        std::string const ns = Prefixed("big_segments_metadata");
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{ns});
        request.AddItem("key", Aws::DynamoDB::Model::AttributeValue{ns});

        Aws::DynamoDB::Model::AttributeValue value;
        value.SetN(std::to_string(millis));
        request.AddItem("synchronizedOn", value);

        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB big-segments metadata: "
                   << outcome.GetError().GetMessage();
        }
    }

    // Writes a metadata row that has no `synchronizedOn` attribute at all.
    // The metadata row exists but is incomplete; the spec treats absent
    // sync-time as "never synchronized" rather than an error.
    void PutBigSegmentMetadataWithoutSyncTime() const {
        std::string const ns = Prefixed("big_segments_metadata");
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{ns});
        request.AddItem("key", Aws::DynamoDB::Model::AttributeValue{ns});

        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB big-segments metadata without "
                      "sync time: "
                   << outcome.GetError().GetMessage();
        }
    }

    // Writes a metadata row whose `synchronizedOn` is stored as a String
    // instead of a Number. DynamoDB does not enforce non-key attribute types,
    // so a non-Relay writer can produce this shape; we want the store to
    // surface it as an error rather than silently returning 0.
    void PutMalformedBigSegmentSyncTime() const {
        std::string const ns = Prefixed("big_segments_metadata");
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{ns});
        request.AddItem("key", Aws::DynamoDB::Model::AttributeValue{ns});
        request.AddItem("synchronizedOn",
                        Aws::DynamoDB::Model::AttributeValue{"not-a-number"});

        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put malformed DynamoDB big-segments metadata: "
                   << outcome.GetError().GetMessage();
        }
    }

   private:
    std::string Prefixed(std::string const& base) const {
        if (prefix_.empty()) {
            return base;
        }
        return prefix_ + ":" + base;
    }

    void PutRaw(std::string const& ns,
                std::string const& key,
                std::optional<std::string> const& item_attribute) const {
        Aws::DynamoDB::Model::PutItemRequest request;
        request.SetTableName(table_name_);
        request.AddItem("namespace", Aws::DynamoDB::Model::AttributeValue{ns});
        request.AddItem("key", Aws::DynamoDB::Model::AttributeValue{key});
        if (item_attribute) {
            request.AddItem(
                "item", Aws::DynamoDB::Model::AttributeValue{*item_attribute});
        }
        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB item ns=" << ns << " key=" << key
                   << ": " << outcome.GetError().GetMessage();
        }
    }

    Aws::DynamoDB::DynamoDBClient& client_;
    std::string const prefix_;
    std::string const table_name_;
};
