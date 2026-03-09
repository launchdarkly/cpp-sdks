#include <gtest/gtest.h>

#include "parser.hpp"

#include <vector>
#include <string>
#include <optional>

using namespace launchdarkly::sse::detail;

// Helper to create a parser and parse SSE data
class ParserTestHelper {
private:
    std::vector<launchdarkly::sse::Event> events_;

    // Lambda that captures events_ by reference
    std::function<void(launchdarkly::sse::Event)> event_handler_;

    EventBody<std::function<void(launchdarkly::sse::Event)>>::value_type value_;
    EventBody<std::function<void(launchdarkly::sse::Event)>>::reader reader_;

public:
    ParserTestHelper()
        : events_()
        , event_handler_([this](launchdarkly::sse::Event event) {
            events_.push_back(std::move(event));
          })
        , value_()
        , reader_(value_) {
        value_.on_event(event_handler_);
        reader_.init();
    }

    void parse(std::string_view data) {
        reader_.put(data);
    }

    std::vector<launchdarkly::sse::Event> const& events() const {
        return events_;
    }

    std::optional<std::string> last_event_id() const {
        return value_.last_event_id();
    }
};

// Basic Event Tests
TEST(ParserTests, ParsesSingleDataField) {
    ParserTestHelper helper;
    helper.parse("data: hello\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("message", helper.events()[0].type());
    EXPECT_EQ("hello", helper.events()[0].data());
}

TEST(ParserTests, ParsesEventWithType) {
    ParserTestHelper helper;
    helper.parse("event: custom\ndata: hello\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("custom", helper.events()[0].type());
    EXPECT_EQ("hello", helper.events()[0].data());
}

TEST(ParserTests, ParsesEventWithId) {
    ParserTestHelper helper;
    helper.parse("id: 123\ndata: hello\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("message", helper.events()[0].type());
    EXPECT_EQ("hello", helper.events()[0].data());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("123", helper.events()[0].id().value());
}

TEST(ParserTests, ParsesEventWithAllFields) {
    ParserTestHelper helper;
    helper.parse("event: update\nid: 456\ndata: test data\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("update", helper.events()[0].type());
    EXPECT_EQ("test data", helper.events()[0].data());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("456", helper.events()[0].id().value());
}

// Multi-line Data Tests
TEST(ParserTests, ParsesMultiLineData) {
    ParserTestHelper helper;
    helper.parse("data: line1\ndata: line2\ndata: line3\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("line1\nline2\nline3", helper.events()[0].data());
}

TEST(ParserTests, ParsesEmptyDataField) {
    ParserTestHelper helper;
    helper.parse("data\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("", helper.events()[0].data());
}

TEST(ParserTests, ParsesDataFieldWithEmptyValue) {
    ParserTestHelper helper;
    helper.parse("data:\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("", helper.events()[0].data());
}

// Line Ending Tests
TEST(ParserTests, HandlesLFLineEndings) {
    ParserTestHelper helper;
    helper.parse("data: test\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

TEST(ParserTests, HandlesCRLFLineEndings) {
    ParserTestHelper helper;
    helper.parse("data: test\r\n\r\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

TEST(ParserTests, HandlesCRLineEndings) {
    ParserTestHelper helper;
    helper.parse("data: test\r\r");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

TEST(ParserTests, HandlesMixedLineEndings) {
    ParserTestHelper helper;
    helper.parse("data: test1\r\ndata: test2\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test1\ntest2", helper.events()[0].data());
}

// Comment Tests
TEST(ParserTests, ParsesCommentLine) {
    ParserTestHelper helper;
    helper.parse(": this is a comment\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("comment", helper.events()[0].type());
    EXPECT_EQ(" this is a comment", helper.events()[0].data());
}

TEST(ParserTests, CommentsDoNotTriggerEventDispatch) {
    ParserTestHelper helper;
    helper.parse(": comment\ndata: test\n\n");

    // Comment should be dispatched immediately, then the data event
    ASSERT_EQ(2, helper.events().size());
    EXPECT_EQ("comment", helper.events()[0].type());
    EXPECT_EQ("test", helper.events()[1].data());
}

TEST(ParserTests, EmptyCommentLine) {
    ParserTestHelper helper;
    helper.parse(":\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("comment", helper.events()[0].type());
    EXPECT_EQ("", helper.events()[0].data());
}

// Field Parsing Tests
TEST(ParserTests, ParsesFieldWithoutSpace) {
    ParserTestHelper helper;
    helper.parse("data:no space\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("no space", helper.events()[0].data());
}

TEST(ParserTests, ParsesFieldWithSpace) {
    ParserTestHelper helper;
    helper.parse("data: with space\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("with space", helper.events()[0].data());
}

TEST(ParserTests, ParsesFieldWithMultipleSpaces) {
    ParserTestHelper helper;
    helper.parse("data:  multiple spaces\n\n");

    ASSERT_EQ(1, helper.events().size());
    // Only the first space after colon is removed
    EXPECT_EQ(" multiple spaces", helper.events()[0].data());
}

TEST(ParserTests, ParsesFieldWithColonInValue) {
    ParserTestHelper helper;
    helper.parse("data: value:with:colons\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("value:with:colons", helper.events()[0].data());
}

TEST(ParserTests, ParsesFieldWithoutColon) {
    ParserTestHelper helper;
    helper.parse("data\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("", helper.events()[0].data());
}

// ID Field Tests
TEST(ParserTests, IdPersistsAcrossEvents) {
    ParserTestHelper helper;
    helper.parse("id: 100\ndata: first\n\n");
    helper.parse("data: second\n\n");

    ASSERT_EQ(2, helper.events().size());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("100", helper.events()[0].id().value());
    ASSERT_TRUE(helper.events()[1].id().has_value());
    EXPECT_EQ("100", helper.events()[1].id().value());
}

TEST(ParserTests, IdUpdatesForSubsequentEvents) {
    ParserTestHelper helper;
    helper.parse("id: 1\ndata: first\n\n");
    helper.parse("id: 2\ndata: second\n\n");

    ASSERT_EQ(2, helper.events().size());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("1", helper.events()[0].id().value());
    ASSERT_TRUE(helper.events()[1].id().has_value());
    EXPECT_EQ("2", helper.events()[1].id().value());
}

TEST(ParserTests, IdWithNullByteIsIgnored) {
    ParserTestHelper helper;
    helper.parse("id: valid\ndata: first\n\n");
    std::string id_with_null = "id: has";
    id_with_null += '\0';
    id_with_null += "null\ndata: second\n\n";
    helper.parse(id_with_null);

    ASSERT_EQ(2, helper.events().size());
    // First event has valid ID
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("valid", helper.events()[0].id().value());
    // Second event should keep the previous valid ID
    ASSERT_TRUE(helper.events()[1].id().has_value());
    EXPECT_EQ("valid", helper.events()[1].id().value());
}

TEST(ParserTests, EmptyIdField) {
    ParserTestHelper helper;
    helper.parse("id:\ndata: test\n\n");

    ASSERT_EQ(1, helper.events().size());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("", helper.events()[0].id().value());
}

// Multiple Events Tests
TEST(ParserTests, ParsesMultipleEvents) {
    ParserTestHelper helper;
    helper.parse("data: first\n\ndata: second\n\ndata: third\n\n");

    ASSERT_EQ(3, helper.events().size());
    EXPECT_EQ("first", helper.events()[0].data());
    EXPECT_EQ("second", helper.events()[1].data());
    EXPECT_EQ("third", helper.events()[2].data());
}

TEST(ParserTests, ParsesEventsWithMultipleEmptyLines) {
    ParserTestHelper helper;
    helper.parse("data: test\n\n\n\n");

    // Multiple empty lines should only trigger one event
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

// Buffering Tests
TEST(ParserTests, HandlesDataSplitAcrossMultiplePuts) {
    ParserTestHelper helper;
    helper.parse("data: hel");
    EXPECT_EQ(0, helper.events().size());

    helper.parse("lo\n\n");
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("hello", helper.events()[0].data());
}

TEST(ParserTests, HandlesFieldNameSplitAcrossMultiplePuts) {
    ParserTestHelper helper;
    helper.parse("da");
    EXPECT_EQ(0, helper.events().size());

    helper.parse("ta: test\n\n");
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

TEST(ParserTests, HandlesLineEndingSplitAcrossMultiplePuts) {
    ParserTestHelper helper;
    helper.parse("data: test\r");
    EXPECT_EQ(0, helper.events().size());

    helper.parse("\ndata: more\n\n");
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test\nmore", helper.events()[0].data());
}

TEST(ParserTests, HandlesEventBoundarySplitAcrossMultiplePuts) {
    ParserTestHelper helper;
    helper.parse("data: first\n");
    EXPECT_EQ(0, helper.events().size());

    helper.parse("\ndata: second\n\n");
    ASSERT_EQ(2, helper.events().size());
    EXPECT_EQ("first", helper.events()[0].data());
    EXPECT_EQ("second", helper.events()[1].data());
}

// Edge Cases
TEST(ParserTests, EmptyInput) {
    ParserTestHelper helper;
    helper.parse("");

    EXPECT_EQ(0, helper.events().size());
}

TEST(ParserTests, OnlyEmptyLines) {
    ParserTestHelper helper;
    helper.parse("\n\n\n");

    // Empty lines without any data should not produce events
    EXPECT_EQ(0, helper.events().size());
}

TEST(ParserTests, EventTypeDefaultsToMessage) {
    ParserTestHelper helper;
    helper.parse("data: test\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("message", helper.events()[0].type());
}

TEST(ParserTests, EventTypeResetsToMessageAfterEvent) {
    ParserTestHelper helper;
    helper.parse("event: custom\ndata: first\n\n");
    helper.parse("data: second\n\n");

    ASSERT_EQ(2, helper.events().size());
    EXPECT_EQ("custom", helper.events()[0].type());
    EXPECT_EQ("message", helper.events()[1].type());
}

TEST(ParserTests, UnknownFieldsAreIgnored) {
    ParserTestHelper helper;
    helper.parse("unknown: field\ndata: test\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

TEST(ParserTests, RetryFieldIsIgnored) {
    ParserTestHelper helper;
    helper.parse("retry: 1000\ndata: test\n\n");

    // Retry field is documented as not implemented
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].data());
}

// Trailing Newline Tests
TEST(ParserTests, DataTrimsTrailingNewline) {
    ParserTestHelper helper;
    helper.parse("data: test\ndata: more\n\n");

    ASSERT_EQ(1, helper.events().size());
    // Each data field appends a newline, but the last one should be trimmed
    EXPECT_EQ("test\nmore", helper.events()[0].data());
}

TEST(ParserTests, SingleDataFieldTrimsTrailingNewline) {
    ParserTestHelper helper;
    helper.parse("data: single\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("single", helper.events()[0].data());
}

// Complex Scenarios
TEST(ParserTests, ComplexEventStream) {
    ParserTestHelper helper;
    helper.parse(": comment\n");
    helper.parse("id: event1\n");
    helper.parse("event: update\n");
    helper.parse("data: first line\n");
    helper.parse("data: second line\n");
    helper.parse("\n");
    helper.parse("data: next event\n");
    helper.parse("\n");

    // Comment + 2 events
    ASSERT_EQ(3, helper.events().size());

    EXPECT_EQ("comment", helper.events()[0].type());

    EXPECT_EQ("update", helper.events()[1].type());
    EXPECT_EQ("first line\nsecond line", helper.events()[1].data());
    ASSERT_TRUE(helper.events()[1].id().has_value());
    EXPECT_EQ("event1", helper.events()[1].id().value());

    EXPECT_EQ("message", helper.events()[2].type());
    EXPECT_EQ("next event", helper.events()[2].data());
    ASSERT_TRUE(helper.events()[2].id().has_value());
    EXPECT_EQ("event1", helper.events()[2].id().value());
}

TEST(ParserTests, RealWorldEventFormat) {
    ParserTestHelper helper;
    std::string event =
        "id: msg-123\n"
        "event: message\n"
        "data: {\"type\":\"update\",\"value\":42}\n"
        "\n";
    helper.parse(event);

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("message", helper.events()[0].type());
    EXPECT_EQ("{\"type\":\"update\",\"value\":42}", helper.events()[0].data());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("msg-123", helper.events()[0].id().value());
}

TEST(ParserTests, DataWithLeadingWhitespace) {
    ParserTestHelper helper;
    helper.parse("data:    spaces\n\n");

    ASSERT_EQ(1, helper.events().size());
    // Only first space after colon is removed
    EXPECT_EQ("   spaces", helper.events()[0].data());
}

TEST(ParserTests, DataWithTrailingWhitespace) {
    ParserTestHelper helper;
    helper.parse("data: trailing    \n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("trailing    ", helper.events()[0].data());
}

TEST(ParserTests, VeryLongDataField) {
    ParserTestHelper helper;
    std::string long_data(10000, 'x');
    helper.parse("data: " + long_data + "\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ(long_data, helper.events()[0].data());
}

TEST(ParserTests, ManyDataFields) {
    ParserTestHelper helper;
    std::string input;
    for (int i = 0; i < 1000; i++) {
        input += "data: line\n";
    }
    input += "\n";
    helper.parse(input);

    ASSERT_EQ(1, helper.events().size());
    // 999 newlines between 1000 "line"s
    std::string expected;
    for (int i = 0; i < 999; i++) {
        expected += "line\n";
    }
    expected += "line";
    EXPECT_EQ(expected, helper.events()[0].data());
}

// Edge Case: Empty Data Tests
TEST(ParserTests, EventWithOnlyEmptyDataFields) {
    ParserTestHelper helper;

    helper.parse("data:\ndata:\ndata:\n\n");

    ASSERT_EQ(1, helper.events().size());
    // Each empty data field appends a newline, last one should be trimmed
    EXPECT_EQ("\n\n", helper.events()[0].data());
}

TEST(ParserTests, EventWithNoDataFields) {
    ParserTestHelper helper;
    helper.parse("event: test\n\n");

    // An event without data should still be dispatched
    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("test", helper.events()[0].type());
    EXPECT_EQ("", helper.events()[0].data());
}

TEST(ParserTests, EventWithIdButNoData) {
    ParserTestHelper helper;
    // Event with ID but no data - tests empty data edge case
    helper.parse("id: 123\nevent: custom\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("custom", helper.events()[0].type());
    EXPECT_EQ("", helper.events()[0].data());
    ASSERT_TRUE(helper.events()[0].id().has_value());
    EXPECT_EQ("123", helper.events()[0].id().value());
}

// Direct test for trim_trailing_newline bug
TEST(ParserTests, DirectTrimTrailingNewlineBugTest) {
    ParserTestHelper helper;
    // Event with only an event type field - no data at all
    helper.parse("event: empty\n\n");

    ASSERT_EQ(1, helper.events().size());
    EXPECT_EQ("empty", helper.events()[0].type());
    EXPECT_EQ("", helper.events()[0].data());
}

TEST(ParserTests, SingleCarriageReturnAtEnd) {
    ParserTestHelper helper;
    // A single CR at the end
    helper.parse("data: test\r");

    // No event yet because no double line ending
    EXPECT_EQ(0, helper.events().size());
}

TEST(ParserTests, SingleLineFeedAtEnd) {
    ParserTestHelper helper;
    // A single LF at the end
    helper.parse("data: test\n");

    // No event yet
    EXPECT_EQ(0, helper.events().size());
}
