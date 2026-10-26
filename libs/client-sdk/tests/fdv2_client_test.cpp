#include <gtest/gtest.h>

#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace std::chrono_literals;

namespace {

class TestPersistence : public IPersistence {
   public:
    using StoreType =
        std::map<std::string,
                 std::map<std::string, std::optional<std::string>>>;

    explicit TestPersistence(StoreType store) : store_(std::move(store)) {}

    void Set(std::string storageNamespace,
             std::string key,
             std::string data) noexcept override {
        std::lock_guard lock{mutex_};
        store_[storageNamespace][key] = data;
    }

    void Remove(std::string storageNamespace,
                std::string key) noexcept override {
        std::lock_guard lock{mutex_};
        store_[storageNamespace].erase(key);
    }

    std::optional<std::string> Read(std::string storageNamespace,
                                    std::string key) noexcept override {
        std::lock_guard lock{mutex_};
        auto const ns = store_.find(storageNamespace);
        if (ns == store_.end()) {
            return std::nullopt;
        }
        auto const entry = ns->second.find(key);
        if (entry == ns->second.end()) {
            return std::nullopt;
        }
        return entry->second;
    }

   private:
    // The SDK reads from its own thread while the test writes from the main
    // one.
    std::mutex mutex_;
    StoreType store_;
};

// Offline mode makes no requests, so the whole FDv2 path can be exercised
// without a service to talk to.
Config OfflineFDv2Config(std::shared_ptr<IPersistence> persistence) {
    auto builder = ConfigBuilder("the-key");
    builder.DataSource().Method(
        DataSourceBuilder::FDv2().InitialMode(ConnectionMode::kOffline));
    builder.Events().Disable();
    if (persistence) {
        builder.Persistence().Custom(std::move(persistence));
    } else {
        builder.Persistence().None();
    }
    return builder.Build().value();
}

// The namespace and context id the client derives for SDK key "the-key" and
// context user:user-key.
char const* const kEnvironment =
    "LaunchDarkly_rUTcjlHPv6Vegd27YmtGYkEGkEUGaEbn5M0JYTFQUpA=";
char const* const kContextId = "CEXjZY7cHJG_ydFy7q4-YEFwVrG3_pkJwA4FAjrbfx0=";

}  // namespace

// The cache is the only source offline mode has, so a miss still starts the
// SDK with no flags, evaluating to defaults.
TEST(FDv2ClientTest, OfflineModeStartsWithNoCachedData) {
    Client client(OfflineFDv2Config(nullptr),
                  ContextBuilder().Kind("user", "user-key").Build());

    auto started = client.StartAsync();

    ASSERT_EQ(std::future_status::ready, started.wait_for(5s));
    EXPECT_TRUE(started.get());
    EXPECT_TRUE(client.Initialized());
    EXPECT_TRUE(client.AllFlags().empty());
}

TEST(FDv2ClientTest, OfflineModeEvaluatesAgainstTheCache) {
    auto persistence =
        std::make_shared<TestPersistence>(TestPersistence::StoreType{
            {kEnvironment,
             {{kContextId, R"({"treat":{"version":1,"value":"fish"}})"}}}});

    Client client(OfflineFDv2Config(persistence),
                  ContextBuilder().Kind("user", "user-key").Build());

    auto started = client.StartAsync();

    ASSERT_EQ(std::future_status::ready, started.wait_for(5s));
    EXPECT_TRUE(started.get());
    EXPECT_EQ("fish", client.StringVariation("treat", "chicken"));
}
