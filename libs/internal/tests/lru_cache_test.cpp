#include "launchdarkly/events/detail/lru_cache.hpp"
#include <gtest/gtest.h>

using namespace launchdarkly::events::detail;

TEST(ContextKeyCacheTests, CacheSizeOne) {
    LRUCache cache(1);

    auto keys = {"foo", "bar", "baz", "qux"};
    for (auto const& k : keys) {
        ASSERT_FALSE(cache.Notice(k));
        ASSERT_EQ(cache.Size(), 1);
    }
}

TEST(ContextKeyCacheTests, CacheIsCleared) {
    LRUCache cache(3);
    auto keys = {"foo", "bar", "baz"};
    for (auto const& k : keys) {
        cache.Notice(k);
    }
    ASSERT_EQ(cache.Size(), 3);
    cache.Clear();
    ASSERT_EQ(cache.Size(), 0);
}

TEST(ContextKeyCacheTests, LRUProperty) {
    LRUCache cache(3);
    auto keys = {"foo", "bar", "baz"};
    for (auto const& k : keys) {
        cache.Notice(k);
    }

    for (auto const& k : keys) {
        ASSERT_TRUE(cache.Notice(k));
    }

    // Evict foo.
    cache.Notice("qux");
    ASSERT_TRUE(cache.Notice("bar"));
    ASSERT_TRUE(cache.Notice("baz"));
    ASSERT_TRUE(cache.Notice("qux"));

    // Evict bar.
    cache.Notice("foo");
    ASSERT_TRUE(cache.Notice("baz"));
    ASSERT_TRUE(cache.Notice("qux"));
    ASSERT_TRUE(cache.Notice("foo"));
}

TEST(ContextKeyCacheTests, DoesNotExceedCapacity) {
    const std::size_t CAP = 100;
    const std::size_t N = 100000;
    LRUCache cache(CAP);

    for (int i = 0; i < N; ++i) {
        cache.Notice(std::to_string(i));
    }

    for (int i = N - CAP; i < N; ++i) {
        ASSERT_TRUE(cache.Notice(std::to_string(i)));
    }

    ASSERT_EQ(cache.Size(), CAP);
}
