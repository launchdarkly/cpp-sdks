#include <gtest/gtest.h>

#include <data_sources/fdv2/fdv2_changeset_translation.hpp>

#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/json.hpp>

using namespace launchdarkly;
using namespace launchdarkly::data_model;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;

// A flag-eval object on the wire. It has a flagVersion but no version. The
// enclosing put-object envelope carries the version instead.
static char const* const kFlagEvalJson =
    R"({"value":"a","variation":1,"flagVersion":5,"trackEvents":true})";

// Object with its own version, which is overridden by the envelope's.
static char const* const kFlagEvalJsonWithVersion =
    R"({"value":"a","variation":1,"version":99,"trackEvents":true})";

static Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

static FDv2Change Put(std::string kind,
                      std::string key,
                      std::uint64_t version,
                      char const* json) {
    return FDv2Change{FDv2Change::ChangeType::kPut, std::move(kind),
                      std::move(key), version, boost::json::parse(json)};
}

static FDv2Change Delete(std::string kind,
                         std::string key,
                         std::uint64_t version) {
    return FDv2Change{FDv2Change::ChangeType::kDelete,
                      std::move(kind),
                      std::move(key),
                      version,
                      {}};
}

TEST(ClientFDv2ChangeSetTranslationTest, NoneChangeSetCarriesNoData) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kNone, {}, Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, ChangeSetType::kNone);
    EXPECT_TRUE(result->data.empty());
}

TEST(ClientFDv2ChangeSetTranslationTest, TypeAndSelectorCarryThrough) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kPartial, {}, Selector{Selector::State{7, "state-7"}}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, ChangeSetType::kPartial);
    ASSERT_TRUE(result->selector.value.has_value());
    EXPECT_EQ(result->selector.value->version, 7);
    EXPECT_EQ(result->selector.value->state, "state-7");
}

TEST(ClientFDv2ChangeSetTranslationTest, PutFlagEvalProducesEvaluationResult) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {Put("flag-eval", "my-flag", 12, kFlagEvalJson)},
                      Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
    ASSERT_TRUE(result->data[0].item.item.has_value());
    EXPECT_EQ(result->data[0].item.item->Detail().Value(), Value("a"));
    EXPECT_EQ(result->data[0].item.item->Detail().VariationIndex(), 1);
    EXPECT_TRUE(result->data[0].item.item->TrackEvents());
}

TEST(ClientFDv2ChangeSetTranslationTest, PutTakesTheEnvelopeVersion) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {Put("flag-eval", "my-flag", 12, kFlagEvalJson)},
                      Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].item.version, 12u);
    ASSERT_TRUE(result->data[0].item.item.has_value());
    EXPECT_EQ(result->data[0].item.item->Version(), 12u);
}

TEST(ClientFDv2ChangeSetTranslationTest, EnvelopeVersionOverridesTheObjects) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kFull,
        {Put("flag-eval", "my-flag", 12, kFlagEvalJsonWithVersion)},
        Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].item.version, 12u);
    ASSERT_TRUE(result->data[0].item.item.has_value());
    EXPECT_EQ(result->data[0].item.item->Version(), 12u);
}

TEST(ClientFDv2ChangeSetTranslationTest, PutOfUnknownKindIsSkipped) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {Put("segment", "my-seg", 1, R"({"key":"my-seg"})"),
                       Put("flag-eval", "my-flag", 12, kFlagEvalJson)},
                      Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
}

TEST(ClientFDv2ChangeSetTranslationTest, PutOfNullObjectIsSkipped) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {Put("flag-eval", "my-flag", 12, "null")},
                      Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->data.empty());
}

TEST(ClientFDv2ChangeSetTranslationTest, PutThatFailsToDeserializeAbandonsAll) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {Put("flag-eval", "good", 12, kFlagEvalJson),
                       Put("flag-eval", "bad", 13, R"(["not-an-object"])")},
                      Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    EXPECT_FALSE(result.has_value());
}

TEST(ClientFDv2ChangeSetTranslationTest, DeleteOfFlagEvalKindTombstones) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kPartial, {Delete("flag-eval", "gone", 42)}, Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "gone");
    EXPECT_EQ(result->data[0].item.version, 42u);
    EXPECT_FALSE(result->data[0].item.item.has_value());
}

TEST(ClientFDv2ChangeSetTranslationTest, DeleteOfUnknownKindIsSkipped) {
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kPartial, {Delete("segment", "gone", 42)}, Selector{}};
    auto result = TranslateChangeSet(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->data.empty());
}
