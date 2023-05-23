#include <gtest/gtest.h>

#include <string>
#include <tuple>

#include <launchdarkly/attribute_reference.hpp>

using launchdarkly::AttributeReference;

class BadReferencesTestFixture : public ::testing::TestWithParam<std::string> {
};

TEST_P(BadReferencesTestFixture, InvalidAttributeReferences) {
    auto str = GetParam();
    auto ref = AttributeReference::FromReferenceStr(std::move(str));
    EXPECT_FALSE(ref.Valid());
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
    auto ref = AttributeReference::FromReferenceStr(str);
    EXPECT_EQ(components.size(), ref.Depth());
    for (auto index = 0; index < ref.Depth(); index++) {
        EXPECT_EQ(components[index], ref.Component(index));
    }
    EXPECT_TRUE(ref.Valid());
    EXPECT_EQ(str, ref.RedactionName());
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
    EXPECT_TRUE(AttributeReference::FromReferenceStr("kind").IsKind());
    EXPECT_TRUE(AttributeReference::FromReferenceStr("/kind").IsKind());
    EXPECT_TRUE(AttributeReference::FromLiteralStr("kind").IsKind());
}

class LiteralsTestFixture
    : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(LiteralsTestFixture, LiteralsAreHandledCorrectly) {
    auto [str, redaction_name] = GetParam();
    auto ref = AttributeReference::FromLiteralStr(str);
    EXPECT_EQ(1, ref.Depth());
    EXPECT_EQ(redaction_name, ref.RedactionName());
    EXPECT_EQ(str, ref.Component(0));
    EXPECT_TRUE(ref.Valid());
}

INSTANTIATE_TEST_SUITE_P(AttributeReferenceTests,
                         LiteralsTestFixture,
                         testing::Values(std::tuple("/a", "/~1a"),
                                         std::tuple("/a/b", "/~1a~1b"),
                                         std::tuple("/~0", "/~1~00"),
                                         std::tuple("test", "test")));

TEST(AttributeReferenceTests, GetComponentOutOfBounds) {
    EXPECT_EQ("", AttributeReference::FromReferenceStr("a").Component(1));
}

TEST(AttributeReferenceTests, OstreamOperator) {
    std::stringstream stream;
    stream << AttributeReference::FromReferenceStr("/a");
    stream.flush();
    EXPECT_EQ("valid(/a)", stream.str());
    stream.str("");
    stream << AttributeReference::FromReferenceStr("/~");
    EXPECT_EQ("invalid(/~)", stream.str());
}

TEST(AttributeReferenceTests, FromString) {
    AttributeReference ref("/a");
    AttributeReference ref_b(std::string("/b"));

    EXPECT_EQ("a", ref.Component(0));
    EXPECT_EQ("b", ref_b.Component(0));
}

TEST(AttributeReferenceTest, CompareToPath) {
    EXPECT_TRUE(AttributeReference("/a") == std::vector<std::string_view>{"a"});

    auto path = std::vector<std::string_view>{"a", "b"};
    EXPECT_TRUE(AttributeReference("/a/b") == path);

    EXPECT_FALSE(AttributeReference("/a/c") == path);
    EXPECT_TRUE(AttributeReference("/a/c") != path);

    EXPECT_FALSE(AttributeReference("/a/b") != path);
}

TEST(AttributeReferenceTests, CanProduceRefStringsFromPaths) {
    EXPECT_EQ("/a/b", AttributeReference::PathToStringReference(
                          std::vector<std::string_view>{"a", "b"}));

    EXPECT_EQ("/~1a/~0b", AttributeReference::PathToStringReference(
                              std::vector<std::string_view>{"/a", "~b"}));
}
