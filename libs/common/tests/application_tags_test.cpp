
#include "config/detail/application_tags.hpp"

#include <gtest/gtest.h>
#include <tuple>

struct TagTest {
    std::string key;
    std::string value;
    bool valid;
};

class AppTagsFixture : public ::testing::TestWithParam<TagTest> {};

INSTANTIATE_TEST_SUITE_P(
    AppTagsTest,
    AppTagsFixture,
    testing::Values(
        TagTest{"", "abc", false},
        TagTest{" ", "abc", false},
        TagTest{"/", "abc", false},
        TagTest{":", "abcs", false},
        TagTest{"abcABC123.-_", "abc", true},
        TagTest{"abc", "", false},
        TagTest{"abc", " ", false},
        TagTest{"abc", "/", false},
        TagTest{"abc", ":", false},
        TagTest{"abc", "abcABC123.-_", true},
        TagTest{
            "abc",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl",
            true,
        },
        TagTest{"abc",
                "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij"
                "klm",  // > 64 chars
                false}));

TEST_P(AppTagsFixture, ValidTagValues) {
    auto param = GetParam();
    bool valid =
        launchdarkly::config::detail::is_valid_tag(param.key, param.value);
    ASSERT_EQ(valid, param.valid);
}
