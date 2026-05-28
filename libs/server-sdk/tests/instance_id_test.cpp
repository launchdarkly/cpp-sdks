#include <gtest/gtest.h>

#include "instance_id.hpp"

#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <cstddef>
#include <regex>
#include <set>
#include <string>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

namespace {

// Matches a canonical UUID v4 in lowercase hex:
//   xxxxxxxx-xxxx-4xxx-Yxxx-xxxxxxxxxxxx
// where 'Y' is one of 8, 9, a, or b (RFC 4122 variant).
bool IsUuidV4(std::string const& s) {
    static std::regex const re(
        "^[0-9a-f]{8}-"
        "[0-9a-f]{4}-"
        "4[0-9a-f]{3}-"
        "[89ab][0-9a-f]{3}-"
        "[0-9a-f]{12}$");
    return std::regex_match(s, re);
}

}  // namespace

// Spec: SCMP-server-connection-minutes-polling section 1.1 requires the
// X-LaunchDarkly-Instance-Id value to be a v4 UUID.
TEST(InstanceIdTest, GeneratedValueIsUuidV4) {
    auto id = MakeInstanceId();
    ASSERT_FALSE(id.empty()) << "MakeInstanceId returned an empty string";
    EXPECT_TRUE(IsUuidV4(id))
        << "MakeInstanceId returned " << id << " which is not a v4 UUID";
}

// Each invocation must yield a different value; spec requires "the GUID MUST
// be used uniquely for this purpose".
TEST(InstanceIdTest, GeneratedValuesAreUnique) {
    constexpr int kSamples = 100;
    std::set<std::string> seen;
    for (int i = 0; i < kSamples; ++i) {
        auto id = MakeInstanceId();
        ASSERT_FALSE(id.empty());
        EXPECT_TRUE(seen.insert(id).second)
            << "duplicate UUID emitted from MakeInstanceId: " << id;
    }
    EXPECT_EQ(seen.size(), static_cast<std::size_t>(kSamples));
}

// The header name constant must match the spec verbatim. This guards against
// accidental renaming (the header name is part of the wire contract).
TEST(InstanceIdTest, HeaderNameMatchesSpec) {
    EXPECT_STREQ(kInstanceIdHeader, "X-LaunchDarkly-Instance-Id");
}

// Sanity-check that a Client can be constructed; the integration that the
// instance-id header actually ends up on outbound requests is covered by the
// cross-SDK contract test harness (capability: "instance-id").
TEST(InstanceIdTest, ClientConstructsWithInstanceIdHeader) {
    // Building a Client exercises the code path that stamps the instance-id
    // header into the shared HttpProperties. We can't observe the header
    // directly from the public API, so this test simply asserts that the
    // construction succeeds; the spec-level guarantees about the header value
    // are exercised by GeneratedValueIsUuidV4 and GeneratedValuesAreUnique,
    // and the on-the-wire guarantee is covered by the cross-SDK contract test
    // harness (capability: "instance-id").
    Client client(ConfigBuilder("sdk-123").Build().value());
    EXPECT_NE(client.Version(), nullptr);
}
