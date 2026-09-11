#include "fdv2_source_result.hpp"

#include <charconv>
#include <cstdint>
#include <random>

namespace launchdarkly::client_side::data_sources {

namespace {

// Subtracts a value drawn uniformly from [0, ttl/2], so that SDKs which fell
// back within the same window do not all re-attempt FDv2 at once.
std::chrono::seconds Jitter(std::chrono::seconds ttl) {
    static thread_local std::mt19937_64 generator{std::random_device{}()};
    std::uniform_int_distribution<std::chrono::seconds::rep> distribution(
        0, ttl.count() / 2);
    return ttl - std::chrono::seconds(distribution(generator));
}

}  // namespace

FDv1FallbackDirective FDv1FallbackDirective::DefaultTtl() {
    return FDv1FallbackDirective{Jitter(kDefaultTtl)};
}

FDv1FallbackDirective FDv1FallbackDirective::FromServiceTtl(
    std::chrono::seconds ttl) {
    if (ttl > std::chrono::seconds::zero() && ttl <= kDefaultTtl) {
        return FDv1FallbackDirective{ttl};
    }
    return DefaultTtl();
}

FDv1FallbackDirective FDv1FallbackDirective::FromServiceTtl(
    std::string_view ttl) {
    std::uint64_t seconds = 0;
    auto const* begin = ttl.data();
    auto const* end = begin + ttl.size();
    auto const [ptr, ec] = std::from_chars(begin, end, seconds);
    if (ec != std::errc{} || ptr != end) {
        return DefaultTtl();
    }
    return FromServiceTtl(std::chrono::seconds(seconds));
}

}  // namespace launchdarkly::client_side::data_sources
