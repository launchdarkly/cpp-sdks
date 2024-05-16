#include <gtest/gtest.h>

#include <boost/json.hpp>

#include "flag_manager/context_index.hpp"

#include <launchdarkly/context_builder.hpp>

using launchdarkly::ContextBuilder;
using launchdarkly::client_side::flag_manager::ContextIndex;

static std::chrono::time_point<std::chrono::system_clock> MakeTimestamp(
    uint64_t time) {
    return std::chrono::system_clock::time_point{
        std::chrono::milliseconds{time}};
}

static uint64_t MsFromTime(
    std::chrono::time_point<std::chrono::system_clock> time) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               time.time_since_epoch())
        .count();
}

TEST(ContextIndexTests, NoticesContexts) {
    auto context_index = ContextIndex();
    context_index.Notice("a", MakeTimestamp(100));

    EXPECT_EQ(1, context_index.Entries().size());
    EXPECT_EQ(100, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("a", context_index.Entries()[0].id);
}

TEST(ContextIndexTests, UpdatesTimeForContext) {
    auto context_index = ContextIndex();
    context_index.Notice("a", MakeTimestamp(100));

    context_index.Notice("a", MakeTimestamp(500));

    EXPECT_EQ(1, context_index.Entries().size());
    EXPECT_EQ(500, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("a", context_index.Entries()[0].id);
}

TEST(ContextIndexTests, NoticesMultipleContexts) {
    auto context_index = ContextIndex();
    context_index.Notice("a", MakeTimestamp(100));

    context_index.Notice("b", MakeTimestamp(500));

    EXPECT_EQ(2, context_index.Entries().size());
    EXPECT_EQ(100, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("a", context_index.Entries()[0].id);

    EXPECT_EQ(500, MsFromTime(context_index.Entries()[1].timestamp));
    EXPECT_EQ("b", context_index.Entries()[1].id);
}

TEST(ContextIndexTests, DoesNotPruneBelowLimit) {
    auto context_index = ContextIndex();
    context_index.Notice("a", MakeTimestamp(100));

    context_index.Notice("b", MakeTimestamp(500));
    // More than 2.
    context_index.Prune(5);

    EXPECT_EQ(2, context_index.Entries().size());
    EXPECT_EQ(100, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("a", context_index.Entries()[0].id);

    EXPECT_EQ(500, MsFromTime(context_index.Entries()[1].timestamp));
    EXPECT_EQ("b", context_index.Entries()[1].id);
}

TEST(ContextIndexTests, DoesPruneOverLimit) {
    auto context_index = ContextIndex();
    // Insert in non-timestamp order to verify sort.
    context_index.Notice("c", MakeTimestamp(600));

    context_index.Notice("a", MakeTimestamp(100));

    context_index.Notice("b", MakeTimestamp(500));
    // Less than 3.
    context_index.Prune(2);

    EXPECT_EQ(2, context_index.Entries().size());
    EXPECT_EQ(600, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("c", context_index.Entries()[0].id);

    EXPECT_EQ(500, MsFromTime(context_index.Entries()[1].timestamp));
    EXPECT_EQ("b", context_index.Entries()[1].id);
}

TEST(ContextIndexTests, CanSerializeAndDeserialize) {
    auto context_index = ContextIndex();
    context_index.Notice("c", MakeTimestamp(600));
    context_index.Notice("a", MakeTimestamp(100));
    context_index.Notice("b", MakeTimestamp(500));

    auto str = boost::json::serialize(boost::json::value_from(context_index));

    boost::system::error_code error_code;
    auto parsed = boost::json::parse(str, error_code);
    auto deserialized = boost::json::value_to<ContextIndex>(parsed);

    EXPECT_EQ(3, deserialized.Entries().size());

    EXPECT_EQ(600, MsFromTime(context_index.Entries()[0].timestamp));
    EXPECT_EQ("c", context_index.Entries()[0].id);

    EXPECT_EQ(100, MsFromTime(context_index.Entries()[1].timestamp));
    EXPECT_EQ("a", context_index.Entries()[1].id);

    EXPECT_EQ(500, MsFromTime(context_index.Entries()[2].timestamp));
    EXPECT_EQ("b", context_index.Entries()[2].id);
}
