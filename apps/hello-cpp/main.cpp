#include <launchdarkly/client_side/client.hpp>

#include <boost/asio/io_context.hpp>

#include <launchdarkly/context_builder.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace net = boost::asio;  // from <boost/asio.hpp>

using launchdarkly::ContextBuilder;
using launchdarkly::LogLevel;
using launchdarkly::client_side::Client;
using launchdarkly::client_side::ConfigBuilder;
using launchdarkly::client_side::DataSourceBuilder;
using launchdarkly::client_side::PersistenceBuilder;
using launchdarkly::config::shared::builders::LoggingBuilder;

class FilePersistence : public IPersistence {
   public:
    FilePersistence(std::string directory) : directory_(std::move(directory)) {
        std::filesystem::create_directories(directory_);
    }
    void Set(std::string storage_namespace,
             std::string key,
             std::string data) noexcept override {
        try {
            std::ofstream file;
            file.open(MakePath(storage_namespace, key));
            file << data;
            file.close();
        } catch (...) {
            std::cout << "Problem writing" << std::endl;
        }
    }

    void Remove(std::string storage_namespace,
                std::string key) noexcept override {
        std::filesystem::remove(MakePath(storage_namespace, key));
    }

    std::optional<std::string> Read(std::string storage_namespace,
                                    std::string key) noexcept override {
        auto path = MakePath(storage_namespace, key);

        try {
            if (std::filesystem::exists(path)) {
                std::ifstream file(path);
                std::stringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        } catch (...) {
            std::cout << "Problem reading" << std::endl;
        }
        return std::nullopt;
    }

   private:
    std::string MakePath(std::string storage_namespace, std::string key) {
        return directory_ + "/" + storage_namespace + "_" + key;
    }
    std::string directory_;
};

int main() {
    net::io_context ioc;

    char const* key = std::getenv("STG_SDK_KEY");
    if (!key) {
        std::cout << "Set environment variable STG_SDK_KEY to the sdk key"
                  << std::endl;
        return 1;
    }

    auto config_builder = ConfigBuilder(key);

    config_builder.ServiceEndpoints()
        .PollingBaseUrl("http://sdk.launchdarkly.com")
        .StreamingBaseUrl("https://stream.launchdarkly.com")
        .EventsBaseUrl("https://events.launchdarkly.com");
    config_builder.DataSource()
        .Method(
            DataSourceBuilder::Polling().PollInterval(std::chrono::seconds{30}))
        .WithReasons(true)
        .UseReport(true);
    config_builder.Logging().Logging(
        LoggingBuilder::BasicLogging().Level(LogLevel::kDebug));
    config_builder.Events().FlushInterval(std::chrono::seconds(5));
    config_builder.Persistence().Custom(
        std::make_shared<FilePersistence>("ld_persist"));

    auto config = config_builder.Build();
    if (!config) {
        std::cout << config.error();
        return 1;
    }

    Client client(std::move(*config),
                  ContextBuilder().kind("user", "ryan").build());

    auto before_init = client.BoolVariationDetail("my-boolean-flag", false);
    // This should be the cached version from our persistence, if the
    // persistence is populated.
    std::cout << "Before Init Complete: " << *before_init << std::endl;

    std::cout << "Initial Status: " << client.DataSourceStatus().Status()
              << std::endl;

    client.DataSourceStatus().OnDataSourceStatusChange([](auto status) {
        std::cout << "Got status: " << status << std::endl;
    });

    client.FlagNotifier().OnFlagChange("my-boolean-flag", [](auto event) {
        std::cout << "Got flag change: " << *event << std::endl;
    });

    client.WaitForReadySync(std::chrono::seconds(30));

    auto value = client.BoolVariationDetail("my-boolean-flag", false);
    std::cout << "Value was: " << *value << std::endl;
    if (auto reason = value.Reason()) {
        std::cout << "Reason was: " << *reason << std::endl;
    }

    // Sit around.
    std::cout << "Press enter to exit" << std::endl << std::endl;
    std::cin.get();
}
