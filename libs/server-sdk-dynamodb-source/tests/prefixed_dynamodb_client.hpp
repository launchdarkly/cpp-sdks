#pragma once

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <aws/core/utils/Outcome.h>
#include <aws/dynamodb/DynamoDBClient.h>
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

#include <string>

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
        sort_def.SetAttributeType(
            Aws::DynamoDB::Model::ScalarAttributeType::S);
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
            // Ignore the not-found case (test may not have created the table).
            return;
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
            request.AddItem("item",
                            Aws::DynamoDB::Model::AttributeValue{*item_attribute});
        }
        auto outcome = client_.PutItem(request);
        if (!outcome.IsSuccess()) {
            FAIL() << "couldn't put DynamoDB item ns=" << ns
                   << " key=" << key << ": "
                   << outcome.GetError().GetMessage();
        }
    }

    Aws::DynamoDB::DynamoDBClient& client_;
    std::string const prefix_;
    std::string const table_name_;
};
