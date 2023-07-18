#include <gtest/gtest.h>
#include "evaluation/detail/semver_operations.hpp"

using namespace launchdarkly::server_side::evaluation::detail;

TEST(SemVer, DefaultConstruction) {
    SemVer version;
    EXPECT_EQ(version.Major(), 0);
    EXPECT_EQ(version.Minor(), 0);
    EXPECT_EQ(version.Patch(), 0);
    EXPECT_FALSE(version.Prerelease());
}

TEST(SemVer, MinimalVersion) {
    SemVer version{1, 2, 3};
    EXPECT_EQ(version.Major(), 1);
    EXPECT_EQ(version.Minor(), 2);
    EXPECT_EQ(version.Patch(), 3);
    EXPECT_FALSE(version.Prerelease());
}

TEST(SemVer, ParseMinimalVersion) {
    auto version = SemVer::Parse("1.2.3");
    ASSERT_TRUE(version);
    EXPECT_EQ(version->Major(), 1);
    EXPECT_EQ(version->Minor(), 2);
    EXPECT_EQ(version->Patch(), 3);
    EXPECT_FALSE(version->Prerelease());
}

TEST(SemVer, ParsePrereleaseVersion) {
    auto version = SemVer::Parse("1.2.3-alpha.123.foo");
    ASSERT_TRUE(version);
    ASSERT_TRUE(version->Prerelease());

    auto const& pre = *version->Prerelease();
    ASSERT_EQ(pre.size(), 3);
    EXPECT_EQ(pre[0], SemVer::Token("alpha"));
    EXPECT_EQ(pre[1], SemVer::Token(123ull));
    EXPECT_EQ(pre[2], SemVer::Token("foo"));
}

TEST(SemVer, ParseInvalid) {
    ASSERT_FALSE(SemVer::Parse(""));
    ASSERT_FALSE(SemVer::Parse("v1.2.3"));
    ASSERT_FALSE(SemVer::Parse("foo"));
    ASSERT_FALSE(SemVer::Parse("1.2.3 "));
    ASSERT_FALSE(SemVer::Parse("1.2.3.alpha.1"));
    ASSERT_FALSE(SemVer::Parse("1.2.3.4"));
    ASSERT_FALSE(SemVer::Parse("1.2.3-_"));
}

TEST(SemVer, BasicComparison) {
    EXPECT_LT(SemVer::Parse("1.0.0"), SemVer::Parse("2.0.0"));
    ASSERT_LT(SemVer::Parse("1.0.0-alpha.1"), SemVer::Parse("1.0.0"));

    ASSERT_GT(SemVer::Parse("2.0.0"), SemVer::Parse("1.0.0"));
    ASSERT_GT(SemVer::Parse("1.0.0"), SemVer::Parse("1.0.0-alpha.1"));

    ASSERT_EQ(SemVer::Parse("1.0.0"), SemVer::Parse("1.0.0"));

    // Build is irrelevant for comparisons.
    ASSERT_EQ(SemVer::Parse("1.2.3+build12345"), SemVer::Parse("1.2.3"));
    ASSERT_EQ(SemVer::Parse("1.2.3-alpha.1+1234"),
              SemVer::Parse("1.2.3-alpha.1+4567"));
}
