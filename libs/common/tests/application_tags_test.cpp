
#include <gtest/gtest.h>

#include <ostream>
#include <tuple>
#include <algorithm>

#include "launchdarkly/config/detail/builders/app_info_builder.hpp"
#include "launchdarkly/error.hpp"
#include "null_logger.hpp"

using launchdarkly::Error;
using launchdarkly::Logger;
using launchdarkly::LogLevel;

struct TagValidity {
    std::string key;
    std::string value;
    std::optional<launchdarkly::Error> error;
};

class TagValidityFixture : public ::testing::TestWithParam<TagValidity> {};

INSTANTIATE_TEST_SUITE_P(
    AppTagsTest,
    TagValidityFixture,
    testing::Values(
        TagValidity{"", "abc", Error::kConfig_ApplicationInfo_EmptyKeyOrValue},
        TagValidity{" ", "abc",
                    Error::kConfig_ApplicationInfo_InvalidKeyCharacters},
        TagValidity{"/", "abc",
                    Error::kConfig_ApplicationInfo_InvalidKeyCharacters},
        TagValidity{":", "abc",
                    Error::kConfig_ApplicationInfo_InvalidKeyCharacters},
        TagValidity{"abcABC123.-_", "abc", std::nullopt},
        TagValidity{"abc", "", Error::kConfig_ApplicationInfo_EmptyKeyOrValue},
        TagValidity{"abc", " ",
                    Error::kConfig_ApplicationInfo_InvalidValueCharacters},
        TagValidity{"abc", "/",
                    Error::kConfig_ApplicationInfo_InvalidValueCharacters},
        TagValidity{"abc", ":",
                    Error::kConfig_ApplicationInfo_InvalidValueCharacters},
        TagValidity{"abc", "abcABC123.-_", std::nullopt},
        TagValidity{
            "abc",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijkl",
            std::nullopt,
        },
        TagValidity{
            "abc",
            "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghij"
            "klm",  // > 64 chars
            Error::kConfig_ApplicationInfo_ValueTooLong}));

TEST_P(TagValidityFixture, ValidTagValues) {
    using namespace launchdarkly::config::detail::builders;
    auto param = GetParam();
    std::optional<launchdarkly::Error> maybe_error =
        IsValidTag(param.key, param.value);
    ASSERT_EQ(maybe_error, param.error);
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
    using namespace launchdarkly::config::detail::builders;
    auto params = GetParam();

    auto logger = NullLogger();

    AppInfoBuilder info;
    if (params.app_id) {
        info.Identifier(*params.app_id);
    }
    if (params.app_version) {
        info.Version(*params.app_version);
    }
    ASSERT_EQ(info.Build(logger), params.concat);
}
