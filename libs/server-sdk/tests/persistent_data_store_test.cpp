#include <gtest/gtest.h>

#include <launchdarkly/persistence/persistent_store_core.hpp>

#include "data_store/persistent/persistent_data_store.hpp"

using launchdarkly::persistence::IPersistentStoreCore;
using launchdarkly::server_side::data_store::persistent::PersistentStore;

class TestCore : public IPersistentStoreCore {
   public:
    InitResult Init(OrderedData const& allData) override {
        return InitResult::kSuccess;
    }

    UpsertResult Upsert(
        launchdarkly::persistence::IPersistentKind const& kind,
        std::string const& itemKey,
        launchdarkly::persistence::SerializedItemDescriptor const& item)
        override {
        return UpsertResult::kNotUpdated;
    }

    GetResult Get(launchdarkly::persistence::IPersistentKind const& kind,
                  std::string const& itemKey) const override {
        return launchdarkly::persistence::IPersistentStoreCore::GetResult();
    }

    AllResult All(
        launchdarkly::persistence::IPersistentKind const& kind) const override {
        return launchdarkly::persistence::IPersistentStoreCore::AllResult();
    }

    bool Initialized() const override { return false; }

    std::string const& Description() const override { return description_; }

   private:
    static inline const std::string description_ = "TestCore";
};

TEST(PersistentDataStoreTest, CanInstantiate) {
    //    launchdarkly::server_side::data_store::persistent::PersistentStore::FlagKind
    //        flag_kind;

    auto core = std::make_shared<TestCore>();
    PersistentStore persistent_store(core, std::chrono::seconds{30},
                                     std::nullopt);
}
