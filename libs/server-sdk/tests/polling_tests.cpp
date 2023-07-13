#include <gtest/gtest.h>

#include <iostream>

#include "data_sources/polling_data_source.hpp"
#include "data_store/memory_store.hpp"
#include "launchdarkly/config/shared/defaults.hpp"
#include "launchdarkly/logging/null_logger.hpp"

using namespace launchdarkly;
using namespace launchdarkly::server_side;
using namespace launchdarkly::server_side::data_sources;
using namespace launchdarkly::server_side::data_store;
using namespace config::shared::built;

TEST(PollingTests, DoPoll) {
    auto logger = launchdarkly::logging::NullLogger();
    auto store = std::make_shared<MemoryStore>();
    DataSourceStatusManager manager;

    HttpProperties properties(
        std::chrono::milliseconds{10000}, std::chrono::milliseconds{10000},
        std::chrono::milliseconds{10000}, std::chrono::milliseconds{10000},
        std::map<std::string, std::string>{
            {"authorization", "sdk-95f77c0e-6ce3-440b-9990-0d1cc6e1b34e"}
        });

    auto data_source_config =
        DataSourceConfig<launchdarkly::config::shared::ServerSDK>{launchdarkly::config::shared::Defaults<
            launchdarkly::config::shared::ServerSDK>::PollingConfig()};

    boost::asio::io_context context;
    ServiceEndpoints endpoints("https://sdk.launchdarkly.com", "", "");

    auto polling = std::make_shared<PollingDataSource>(endpoints, data_source_config, properties,
                              context.get_executor(), *store, manager, logger);


    manager.OnDataSourceStatusChange([store](auto status){
        std::cout << "Got status: " << status << std::endl;
        std::cout << store->GetFlag("my-boolean-flag")->item.value().variations << std::endl;
    });

    polling->Start();


    context.run();
}
