#include <gtest/gtest.h>

#include <launchdarkly/server_side/integrations/data_reader/kinds.hpp>
#include <launchdarkly/server_side/integrations/dynamodb/dynamodb_source.hpp>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include "aws_sdk_guard.hpp"
#include "prefixed_dynamodb_client.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/Scheme.h>
#include <aws/dynamodb/DynamoDBClient.h>

#include <boost/json.hpp>

#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

using namespace launchdarkly::server_side::integrations;
using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side;

namespace {

std::string EnvOr(char const* name, std::string const& fallback) {
    char const* value = std::getenv(name);
    if (value && *value) {
        return value;
    }
    return fallback;
}

DynamoDBClientOptions LocalOptions() {
    DynamoDBClientOptions options;
    options.endpoint = EnvOr("LD_DYNAMODB_TEST_ENDPOINT", "http://localhost:8000");
    options.region = EnvOr("LD_DYNAMODB_TEST_REGION", "us-east-1");
    // DynamoDB Local accepts any non-empty credentials.
    options.aws_access_key_id = "dummy";
    options.aws_secret_access_key = "dummy";
    return options;
}

}  // namespace

class DynamoDBTests : public ::testing::Test {
   public:
    DynamoDBTests()
        : table_name_("ld-dynamodb-source-test"),
          prefix_("testprefix"),
          options_(LocalOptions()),
          client_(MakeRawClient()) {}

    void SetUp() override {
        // Reset table state between tests.
        PrefixedDynamoDBClient::DeleteTable(*client_, table_name_);
        PrefixedDynamoDBClient::CreateTable(*client_, table_name_);

        auto maybe_source =
            DynamoDBDataSource::Create(table_name_, prefix_, options_);
        ASSERT_TRUE(maybe_source) << maybe_source.error();
        source = std::move(*maybe_source);
    }

    void TearDown() override {
        source.reset();
        PrefixedDynamoDBClient::DeleteTable(*client_, table_name_);
    }

    void Init() {
        PrefixedDynamoDBClient(*client_, prefix_, table_name_).Init();
    }

    void PutFlag(Flag const& flag) {
        PrefixedDynamoDBClient(*client_, prefix_, table_name_).PutFlag(flag);
    }

    void PutSegment(Segment const& segment) {
        PrefixedDynamoDBClient(*client_, prefix_, table_name_)
            .PutSegment(segment);
    }

    void PutDeletedFlag(std::string const& key, std::string const& ts) {
        PrefixedDynamoDBClient(*client_, prefix_, table_name_)
            .PutDeletedFlag(key, ts);
    }

    void PutDeletedSegment(std::string const& key, std::string const& ts) {
        PrefixedDynamoDBClient(*client_, prefix_, table_name_)
            .PutDeletedSegment(key, ts);
    }

    void WithPrefixedClient(
        std::string const& prefix,
        std::function<void(PrefixedDynamoDBClient const&)> const& f) {
        f(PrefixedDynamoDBClient(*client_, prefix, table_name_));
    }

    void WithPrefixedSource(
        std::string const& prefix,
        std::function<void(DynamoDBDataSource const&)> const& f) const {
        auto maybe_source =
            DynamoDBDataSource::Create(table_name_, prefix, options_);
        ASSERT_TRUE(maybe_source) << maybe_source.error();
        f(**maybe_source);
    }

   protected:
    std::shared_ptr<DynamoDBDataSource> source;
    std::string const table_name_;
    std::string const prefix_;
    DynamoDBClientOptions const options_;

   private:
    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> MakeRawClient() const {
        detail::AwsSdkGuard::Ensure();
        Aws::Client::ClientConfiguration config;
        config.region = *options_.region;
        config.endpointOverride = *options_.endpoint;
        if (options_.endpoint->rfind("http://", 0) == 0) {
            config.scheme = Aws::Http::Scheme::HTTP;
            config.verifySSL = false;
        }
        Aws::Auth::AWSCredentials creds{*options_.aws_access_key_id,
                                        *options_.aws_secret_access_key};
        return std::make_unique<Aws::DynamoDB::DynamoDBClient>(creds, config);
    }

    std::unique_ptr<Aws::DynamoDB::DynamoDBClient> client_;
};

TEST_F(DynamoDBTests, EmptyIsNotInitialized) {
    ASSERT_FALSE(source->Initialized());

    auto all_flags = source->All(FlagKind{});
    ASSERT_TRUE(all_flags.has_value());
    ASSERT_TRUE(all_flags->empty());

    auto all_segments = source->All(SegmentKind{});
    ASSERT_TRUE(all_segments.has_value());
    ASSERT_TRUE(all_segments->empty());
}

TEST_F(DynamoDBTests, ChecksInitialized) {
    ASSERT_FALSE(source->Initialized());
    Init();
    ASSERT_TRUE(source->Initialized());
}

TEST_F(DynamoDBTests, GetFlag) {
    Flag const flag{"foo", 1, true};
    PutFlag(flag);

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, serialize(boost::json::value_from(flag)));
    } else {
        FAIL() << "expected flag to be found";
    }
}

TEST_F(DynamoDBTests, GetSegment) {
    Segment const segment{"foo", 1};
    PutSegment(segment);

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem,
                  serialize(boost::json::value_from(segment)));
    } else {
        FAIL() << "expected segment to be found";
    }
}

TEST_F(DynamoDBTests, GetMissingFlag) {
    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(DynamoDBTests, GetMissingSegment) {
    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(DynamoDBTests, GetDeletedFlag) {
    PutDeletedFlag("foo", "foo_tombstone");

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, "foo_tombstone");
    } else {
        FAIL() << "expected tombstone to be present";
    }
}

TEST_F(DynamoDBTests, GetDeletedSegment) {
    PutDeletedSegment("foo", "foo_tombstone");

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);

    if (auto const f = *result) {
        ASSERT_EQ(f->serializedItem, "foo_tombstone");
    } else {
        FAIL() << "expected tombstone to be present";
    }
}

TEST_F(DynamoDBTests, GetFlagDoesNotFindSegment) {
    PutSegment(Segment{"foo", 1});

    auto const result = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(DynamoDBTests, GetSegmentDoesNotFindFlag) {
    PutFlag(Flag{"foo", 1, true});

    auto const result = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(result);
    ASSERT_FALSE(*result);
}

TEST_F(DynamoDBTests, GetAllFlagsWhenEmpty) {
    auto const result = source->All(FlagKind{});
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->empty());
}

TEST_F(DynamoDBTests, GetAllSegmentsWhenEmpty) {
    auto const result = source->All(SegmentKind{});
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->empty());
}

TEST_F(DynamoDBTests, GetAllFlags) {
    Flag const flag1{"foo", 1, true};
    Flag const flag2{"bar", 2, false};

    PutFlag(flag1);
    PutFlag(flag2);
    PutDeletedFlag("baz", "baz_tombstone");

    auto const result = source->All(FlagKind{});
    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), 3);

    auto const& flags = *result;
    auto const flag1_it = flags.find("foo");
    ASSERT_NE(flag1_it, flags.end());
    ASSERT_EQ(flag1_it->second.serializedItem,
              serialize(boost::json::value_from(flag1)));

    auto const flag2_it = flags.find("bar");
    ASSERT_NE(flag2_it, flags.end());
    ASSERT_EQ(flag2_it->second.serializedItem,
              serialize(boost::json::value_from(flag2)));

    auto const flag3_it = flags.find("baz");
    ASSERT_NE(flag3_it, flags.end());
    ASSERT_EQ(flag3_it->second.serializedItem, "baz_tombstone");
}

TEST_F(DynamoDBTests, InitializedPrefixIndependence) {
    WithPrefixedClient("not_our_prefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefixedClient("TestPrefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });

    WithPrefixedClient("stillnotprefix", [&](auto const& client) {
        client.Init();
        ASSERT_FALSE(source->Initialized());
    });
}

TEST_F(DynamoDBTests, SegmentPrefixIndependence) {
    auto MakeSegment = [](std::uint64_t const version) {
        return Segment{"foo", version};
    };

    auto PrefixName = [](std::uint64_t const version) {
        return "prefix" + std::to_string(version);
    };

    auto ValidateSegment = [&](ISerializedDataReader::GetResult const& result,
                               std::size_t i) {
        ASSERT_TRUE(result);
        if (auto const f = *result) {
            ASSERT_EQ(f->serializedItem,
                      serialize(boost::json::value_from(MakeSegment(i))));
        } else {
            FAIL() << "expected segment to be found under " << PrefixName(i);
        }
    };

    constexpr std::size_t kPrefixCount = 10;

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedClient(PrefixName(i), [&](auto const& client) {
            client.PutSegment(MakeSegment(i));
        });
    }

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedSource(PrefixName(i), [&](auto const& src) {
            ValidateSegment(src.Get(SegmentKind{}, "foo"), i);
            auto all = src.All(SegmentKind{});
            ASSERT_TRUE(all);
            ASSERT_EQ(all->size(), 1);
        });
    }
}

TEST_F(DynamoDBTests, FlagPrefixIndependence) {
    auto MakeFlag = [](std::uint64_t const version) {
        return Flag{"foo", version, true};
    };

    auto PrefixName = [](std::uint64_t const version) {
        return "prefix" + std::to_string(version);
    };

    auto ValidateFlag = [&](ISerializedDataReader::GetResult const& result,
                            std::size_t i) {
        ASSERT_TRUE(result);
        if (auto const f = *result) {
            ASSERT_EQ(f->serializedItem,
                      serialize(boost::json::value_from(MakeFlag(i))));
        } else {
            FAIL() << "expected flag to be found under " << PrefixName(i);
        }
    };

    constexpr std::size_t kPrefixCount = 10;

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedClient(PrefixName(i), [&](auto const& client) {
            client.PutFlag(MakeFlag(i));
        });
    }

    for (std::size_t i = 0; i < kPrefixCount; i++) {
        WithPrefixedSource(PrefixName(i), [&](auto const& src) {
            ValidateFlag(src.Get(FlagKind{}, "foo"), i);
            auto all = src.All(FlagKind{});
            ASSERT_TRUE(all);
            ASSERT_EQ(all->size(), 1);
        });
    }
}

TEST_F(DynamoDBTests, FlagAndSegmentCanCoexistWithSameKey) {
    Flag const flag_in{"foo", 1, true};
    Segment const segment_in{"foo", 1};

    PutFlag(flag_in);
    PutSegment(segment_in);

    auto flag = source->Get(FlagKind{}, "foo");
    ASSERT_TRUE(flag);
    ASSERT_EQ((*flag)->serializedItem,
              serialize(boost::json::value_from(flag_in)));

    auto segment = source->Get(SegmentKind{}, "foo");
    ASSERT_TRUE(segment);
    ASSERT_EQ((*segment)->serializedItem,
              serialize(boost::json::value_from(segment_in)));
}

TEST_F(DynamoDBTests, EmptyPrefixUsesBareNamespaces) {
    // A source with no prefix should successfully read rows whose
    // namespace partition keys are bare "features" / "segments" / "$inited".
    WithPrefixedClient("", [&](auto const& client) {
        client.Init();
        client.PutFlag(Flag{"foo", 1, true});
    });

    WithPrefixedSource("", [&](auto const& src) {
        ASSERT_TRUE(src.Initialized());
        auto const got = src.Get(FlagKind{}, "foo");
        ASSERT_TRUE(got);
        ASSERT_TRUE(*got);
    });
}

TEST_F(DynamoDBTests, AllPaginatesAcrossMultiplePages) {
    // DynamoDB Query responses cap at 1MB. Insert enough large flags to
    // force at least two pages, then verify All() returns every one.
    constexpr std::size_t kFlagCount = 40;
    constexpr std::size_t kPayloadBytes = 100 * 1024;  // 100 KiB per flag

    for (std::size_t i = 0; i < kFlagCount; ++i) {
        // Use the deleted-flag path to write an arbitrary opaque payload
        // without forcing the data model to accept giant strings.
        PutDeletedFlag("flag_" + std::to_string(i),
                       std::string(kPayloadBytes, 'x'));
    }

    auto const result = source->All(FlagKind{});
    ASSERT_TRUE(result);
    ASSERT_EQ(result->size(), kFlagCount);
}

TEST_F(DynamoDBTests, IdentityReturnsDynamodb) {
    ASSERT_EQ(source->Identity(), "dynamodb");
}

TEST_F(DynamoDBTests, CanConvertDataSourceToDataReader) {
    auto maybe_source =
        DynamoDBDataSource::Create(table_name_, "prefix", LocalOptions());
    ASSERT_TRUE(maybe_source);

    std::shared_ptr<ISerializedDataReader> reader = std::move(*maybe_source);
}

TEST_F(DynamoDBTests, CanUseAsSDKLazyLoadDataSource) {
    Flag flag_a{"foo", 1, false, std::nullopt, {true, false}};
    flag_a.offVariation = 0;  // variation: true
    Flag flag_b{"bar", 1, false, std::nullopt, {true, false}};
    flag_b.offVariation = 1;  // variation: false

    PutFlag(flag_a);
    PutFlag(flag_b);
    Init();

    auto cfg_builder = ConfigBuilder("sdk-123");
    cfg_builder.DataSystem().Method(
        config::builders::LazyLoadBuilder().Source(source));
    cfg_builder.Events().Disable();
    auto config = cfg_builder.Build();

    ASSERT_TRUE(config);

    auto client = Client(std::move(*config));
    client.StartAsync();

    auto const context =
        launchdarkly::ContextBuilder().Kind("cat", "shadow").Build();

    auto const all_flags = client.AllFlagsState(context);
    auto const expected = std::unordered_map<std::string, launchdarkly::Value>{
        {"foo", true}, {"bar", false}};

    ASSERT_TRUE(all_flags.Valid());
    ASSERT_EQ(all_flags.Values(), expected);
}

TEST(DynamoDBErrorTests, NonExistentTableReturnsErrorFromGet) {
    auto maybe_source = DynamoDBDataSource::Create(
        "table-that-does-not-exist", "prefix", LocalOptions());
    ASSERT_TRUE(maybe_source);

    auto const src = std::move(*maybe_source);

    ASSERT_FALSE(src->Initialized());

    auto const get_flag = src->Get(FlagKind{}, "foo");
    ASSERT_FALSE(get_flag);

    auto const get_segment = src->Get(SegmentKind{}, "foo");
    ASSERT_FALSE(get_segment);

    auto const get_all_flag = src->All(FlagKind{});
    ASSERT_FALSE(get_all_flag);

    auto const get_all_segment = src->All(SegmentKind{});
    ASSERT_FALSE(get_all_segment);
}

TEST(DynamoDBErrorTests, UnreachableEndpointReturnsErrorFromGet) {
    DynamoDBClientOptions options;
    options.endpoint = "http://127.0.0.1:1";  // nothing listening
    options.region = "us-east-1";
    options.aws_access_key_id = "dummy";
    options.aws_secret_access_key = "dummy";

    auto maybe_source =
        DynamoDBDataSource::Create("any-table", "prefix", options);
    ASSERT_TRUE(maybe_source);

    auto const src = std::move(*maybe_source);

    ASSERT_FALSE(src->Initialized());

    auto const get_flag = src->Get(FlagKind{}, "foo");
    ASSERT_FALSE(get_flag);

    auto const get_all = src->All(FlagKind{});
    ASSERT_FALSE(get_all);
}
