#include <gtest/gtest.h>
#include <string>
#include <tuple>

#include "attribute_reference.hpp"

using launchdarkly::AttributeReference;

class BadReferencesTestFixture : public ::testing::TestWithParam<std::string> {
};

TEST_P(BadReferencesTestFixture, InvalidAttributeReferences) {
    auto str = GetParam();
    auto ref = AttributeReference::from_reference_str(std::move(str));
    EXPECT_FALSE(ref.valid());
}

INSTANTIATE_TEST_SUITE_P(
    AttributeReferenceTests,
    BadReferencesTestFixture,
    testing::Values("", "/", "//", "/~~", "/~", "/~2", "/a/"));

class GoodReferencesTestFixture
    : public ::testing::TestWithParam<
          std::tuple<std::string, std::vector<std::string>>> {};

TEST_P(GoodReferencesTestFixture, ValidAttributeReferences) {
    auto [str, components] = GetParam();
    auto ref = AttributeReference::from_reference_str(str);
    EXPECT_EQ(components.size(), ref.depth());
    for (auto index = 0; index < ref.depth(); index++) {
        EXPECT_EQ(components[index], ref.component(index));
    }
    EXPECT_TRUE(ref.valid());
    EXPECT_EQ(str, ref.redaction_name());
}

INSTANTIATE_TEST_SUITE_P(
    AttributeReferenceTests,
    GoodReferencesTestFixture,
    testing::Values(std::tuple("/a", std::vector<std::string>{"a"}),
                    std::tuple("/a/b", std::vector<std::string>{"a", "b"}),
                    std::tuple("/~0", std::vector<std::string>{"~"}),
                    std::tuple("/~1", std::vector<std::string>{"/"}),
                    std::tuple("/~1~0", std::vector<std::string>{"/~"}),
                    std::tuple("/~0~1", std::vector<std::string>{"~/"}),
                    std::tuple(" /a/b", std::vector<std::string>{" /a/b"})));

TEST(AttributeReferenceTests, KindReferences) {
    EXPECT_TRUE(AttributeReference::from_reference_str("kind").is_kind());
    EXPECT_TRUE(AttributeReference::from_reference_str("/kind").is_kind());
    EXPECT_TRUE(AttributeReference::from_literal_str("kind").is_kind());
}

class LiteralsTestFixture
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(LiteralsTestFixture, LiteralsAreHandledCorrectly) {
    auto [str, redaction_name] = GetParam();
    auto ref = AttributeReference::from_literal_str(str);
    EXPECT_EQ(1, ref.depth());
    EXPECT_EQ(redaction_name, ref.redaction_name());
    EXPECT_EQ(str, ref.component(0));
    EXPECT_TRUE(ref.valid());
}

INSTANTIATE_TEST_SUITE_P(AttributeReferenceTests,
                         LiteralsTestFixture,
                         testing::Values(std::tuple("/a", "/~1a"),
                                         std::tuple("/a/b", "/~1a~1b"),
                                         std::tuple("/~0", "/~1~00"),
                                         std::tuple("test", "test")));

TEST(AttributeReferenceTests, GetComponentOutOfBounds) {
    EXPECT_EQ("", AttributeReference::from_reference_str("a").component(1));
}

TEST(AttributeReferenceTests, OstreamOperator) {
    std::stringstream stream;
    stream << AttributeReference::from_reference_str("/a");
    stream.flush();
    EXPECT_EQ("valid(/a)", stream.str());
    stream.str("");
    stream << AttributeReference::from_reference_str("/~");
    EXPECT_EQ("invalid(/~)", stream.str());
}
