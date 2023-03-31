
#include <gtest/gtest.h>
#include <ostream>
#include <tuple>
#include "config/detail/application_info.hpp"

using namespace launchdarkly::config::detail;

struct TagValidity {
    std::string key;
    std::string value;
    bool valid;
};

class TagValidityFixture : public ::testing::TestWithParam<TagValidity> {};

INSTANTIATE_TEST_SUITE_P(
    AppTagsTest,
    TagValidityFixture,
    testing::Values(
        TagValidity{"", "abc", false},
        TagValidity{" ", "abc", false},
        TagValidity{"/", "abc", false},
        TagValidity{":", "abcs", false},
        TagValidity{"abcABC123.-_", "abc", true},
        TagValidity{"abc", "", false},
        TagValidity{"abc", " ", false},
        TagValidity{"abc", "/", false},
        TagValidity{"abc", ":", false},
        TagValidity{"abc", "abcABC123.-_", true},
        TagValidity{
            "abc",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl",
            true,
        },
        TagValidity{
            "abc",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij"
            "klm",  // > 64 chars
            false}));

TEST_P(TagValidityFixture, ValidTagValues) {
    auto param = GetParam();
    bool valid =
        launchdarkly::config::detail::is_valid_tag(param.key, param.value);
    ASSERT_EQ(valid, param.valid);
}

struct TagBuild {
    std::optional<std::string> app_id;
    std::optional<std::string> app_version;
    std::optional<std::string> concat;
    std::string name;
};

class TagBuildFixture : public ::testing::TestWithParam<TagBuild> {};

auto const kInvalidTag = "!*@!#";

INSTANTIATE_TEST_SUITE_P(
    AppTagsTest,
    TagBuildFixture,
    testing::Values(
        TagBuild{std::nullopt, std::nullopt, std::nullopt,
                 "no tags means no output"},
        TagBuild{"microservice-a", "1.0",
                 "application-id/microservice-a application-version/1.0",
                 "multiple tags are ordered correctly"},
        TagBuild{"microservice-a", std::nullopt,
                 "application-id/microservice-a",
                 "single app id tag is output"},
        TagBuild{std::nullopt, "1.0", "application-version/1.0",
                 "single app version tag is output"},
        TagBuild{kInvalidTag, std::nullopt, std::nullopt,
                 "invalid tag is not output"},
        TagBuild{kInvalidTag, kInvalidTag, std::nullopt,
                 "two invalid tags are not output"},
        TagBuild{"microservice-a", kInvalidTag, "application-id/microservice-a",
                 "presence of invalid tag does not prevent valid tag from "
                 "being output"}),
    [](testing::TestParamInfo<TagBuild> const& info) {
        std::string name = info.param.name;
        std::replace_if(
            name.begin(), name.end(), [](char c) { return c == ' '; }, '_');
        return "test" + std::to_string(info.index) + "_" + name;
    });

TEST_P(TagBuildFixture, BuiltTags) {
    auto params = GetParam();

    ApplicationInfo info;
    if (params.app_id) {
        info.app_identifier(*params.app_id);
    }
    if (params.app_version) {
        info.app_version(*params.app_version);
    }
    ASSERT_EQ(info.build(), params.concat);
}
