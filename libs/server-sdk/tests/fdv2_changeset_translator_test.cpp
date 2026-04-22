#include <gtest/gtest.h>

#include <data_systems/fdv2/fdv2_changeset_translator.hpp>

#include <launchdarkly/data_model/change_set.hpp>
#include <launchdarkly/data_model/fdv2_change.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/json.hpp>

using namespace launchdarkly;
using namespace launchdarkly::data_model;
using namespace launchdarkly::server_side::data_interfaces;
using namespace launchdarkly::server_side::data_systems;

// Minimal valid flag JSON accepted by the Flag deserializer.
static char const* const kFlagJson =
    R"({"key":"my-flag","on":true,"fallthrough":{"variation":0},)"
    R"("variations":[true,false],"version":1})";

// Minimal valid segment JSON accepted by the Segment deserializer.
static char const* const kSegmentJson =
    R"({"key":"my-seg","version":2,"rules":[],"included":[],"excluded":[]})";

static Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

// ============================================================================
// kNone changeset
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, NoneChangeSetProducesEmptyTypedChangeSet) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kNone, {}, Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->type, ChangeSetType::kNone);
    EXPECT_TRUE(result->data.empty());
}

// ============================================================================
// Known kinds — put
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, PutFlagProducesTypedFlag) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {FDv2Change{FDv2Change::ChangeType::kPut, "flag",
                                  "my-flag", 1, boost::json::parse(kFlagJson)}},
                      Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
    EXPECT_TRUE(
        std::holds_alternative<ItemDescriptor<Flag>>(result->data[0].object));
}

TEST(FDv2ChangeSetTranslatorTest, PutSegmentProducesTypedSegment) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kFull,
        {FDv2Change{FDv2Change::ChangeType::kPut, "segment", "my-seg", 2,
                    boost::json::parse(kSegmentJson)}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-seg");
    EXPECT_TRUE(std::holds_alternative<ItemDescriptor<Segment>>(
        result->data[0].object));
}

// ============================================================================
// Known kinds — delete
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, DeleteFlagProducesFlagTombstone) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kPartial,
        {FDv2Change{FDv2Change::ChangeType::kDelete, "flag", "my-flag", 5, {}}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
    auto const* desc =
        std::get_if<ItemDescriptor<Flag>>(&result->data[0].object);
    ASSERT_NE(desc, nullptr);
    EXPECT_EQ(desc->version, 5u);
    EXPECT_FALSE(desc->item.has_value());
}

TEST(FDv2ChangeSetTranslatorTest, DeleteSegmentProducesSegmentTombstone) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kPartial,
        {FDv2Change{
            FDv2Change::ChangeType::kDelete, "segment", "my-seg", 3, {}}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    auto const* desc =
        std::get_if<ItemDescriptor<Segment>>(&result->data[0].object);
    ASSERT_NE(desc, nullptr);
    EXPECT_EQ(desc->version, 3u);
    EXPECT_FALSE(desc->item.has_value());
}

// ============================================================================
// Unknown kind — skipped
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, UnknownKindInPutIsSkipped) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kFull,
        {FDv2Change{FDv2Change::ChangeType::kPut, "experiment", "exp-1", 1,
                    boost::json::parse(R"({"key":"exp-1","version":1})")},
         FDv2Change{FDv2Change::ChangeType::kPut, "flag", "my-flag", 1,
                    boost::json::parse(kFlagJson)}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
}

TEST(FDv2ChangeSetTranslatorTest, UnknownKindInDeleteIsSkipped) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kFull,
        {FDv2Change{
             FDv2Change::ChangeType::kDelete, "experiment", "exp-1", 1, {}},
         FDv2Change{FDv2Change::ChangeType::kDelete, "flag", "my-flag", 2, {}}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->data.size(), 1u);
    EXPECT_EQ(result->data[0].key, "my-flag");
}

// ============================================================================
// Null object on put — skipped
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, NullObjectInPutFlagIsSkipped) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {FDv2Change{FDv2Change::ChangeType::kPut, "flag",
                                  "my-flag", 1, boost::json::value{nullptr}}},
                      Selector{}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->data.empty());
}

// ============================================================================
// Deserialization failure — abort
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, MalformedFlagAbortsTranslation) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{ChangeSetType::kFull,
                      {FDv2Change{FDv2Change::ChangeType::kPut, "flag",
                                  "bad-flag", 1, boost::json::parse(R"({})")}},
                      Selector{}};
    auto result = translator.Translate(raw, logger);

    EXPECT_FALSE(result.has_value());
}

TEST(FDv2ChangeSetTranslatorTest, MalformedSegmentAbortsTranslation) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    // A non-empty object missing required fields triggers a schema failure
    // (the deserializer treats an empty object as null/skip, not an error).
    FDv2ChangeSet raw{
        ChangeSetType::kFull,
        {FDv2Change{FDv2Change::ChangeType::kPut, "segment", "bad-seg", 1,
                    boost::json::parse(R"({"key":"bad-seg"})")}},
        Selector{}};
    auto result = translator.Translate(raw, logger);

    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Selector is preserved
// ============================================================================

TEST(FDv2ChangeSetTranslatorTest, SelectorIsPreserved) {
    FDv2ChangeSetTranslator translator;
    auto logger = MakeNullLogger();

    FDv2ChangeSet raw{
        ChangeSetType::kFull, {}, Selector{Selector::State{7, "state-abc"}}};
    auto result = translator.Translate(raw, logger);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->selector.value.has_value());
    EXPECT_EQ(result->selector.value->state, "state-abc");
    EXPECT_EQ(result->selector.value->version, 7);
}
