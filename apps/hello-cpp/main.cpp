#include <launchdarkly/client_side/client.hpp>

#include <boost/asio/io_context.hpp>

#include <launchdarkly/context_builder.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

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
    void SetValue(std::string storage_namespace,
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

    void RemoveValue(std::string storage_namespace,
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

    Client client(
        ConfigBuilder(key)
            .ServiceEndpoints(
                launchdarkly::client_side::EndpointsBuilder()
                    // Set to http to demonstrate redirect to https.
                    .PollingBaseUrl("http://sdk.launchdarkly.com")
                    .StreamingBaseUrl("https://stream.launchdarkly.com")
                    .EventsBaseUrl("https://events.launchdarkly.com"))
            .DataSource(DataSourceBuilder()
                            .Method(DataSourceBuilder::Polling().PollInterval(
                                std::chrono::seconds{30}))
                            .WithReasons(true)
                            .UseReport(true))
            .Logging(LoggingBuilder::BasicLogging().Level(LogLevel::kDebug))
            .Events(launchdarkly::client_side::EventsBuilder().FlushInterval(
                std::chrono::seconds(5)))
            .Persistence(
                PersistenceBuilder(PersistenceBuilder::Custom().Implementation(
                    std::make_unique<FilePersistence>("ld_persist"))))
            .Build()
            .value(),
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

    // Identify a series of contexts.
    for (auto context_index = 0; context_index < 4; context_index++) {
        std::cout << "Identifying user: "
                  << "ryan" << context_index << std::endl;
        auto future = client.IdentifyAsync(
            ContextBuilder()
                .kind("user", "ryan" + std::to_string(context_index))
                .build());
        auto before_ident =
            client.BoolVariationDetail("my-boolean-flag", false);
        future.get();
        auto after_ident = client.BoolVariationDetail("my-boolean-flag", false);

        std::cout << "For: "
                  << "ryan" << context_index << ": "
                  << "Before ident complete: " << *before_init
                  << " After: " << *after_ident << std::endl;

        sleep(1);
    }

    // Sit around.
    std::cout << "Press enter to exit" << std::endl << std::endl;
    std::cin.get();
}
