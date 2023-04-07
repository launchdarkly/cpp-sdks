#include "launchdarkly/detail/streaming_data_source.hpp"
#include <gtest/gtest.h>
#include "console_backend.hpp"
#include "context_builder.hpp"
#include "logger.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>

using namespace launchdarkly;
using namespace launchdarkly::client_side;

class Handler : public IDataSourceUpdateSink {
   public:
    Handler(Logger const& logger) : logger_(logger) {}

    virtual void init(std::map<std::string, ItemDescriptor> data) {
        LD_LOG(logger_, LogLevel::kInfo) << "Init";
    }
    virtual void upsert(std::string key, ItemDescriptor) {
        LD_LOG(logger_, LogLevel::kInfo) << "Upsert";
    }

   private:
    Logger const& logger_;
};

TEST(StreamingDataSourceTests, RunTheDataSource) {
    boost::asio::ssl::context ssl_context{boost::asio::ssl::context::sslv23};
    ssl_context.set_default_verify_paths();

    boost::asio::io_context ioc{1};

    auto context =
        ContextBuilder().kind("user", "user-key").name("Ryan").build();
    bool use_report = false;
    bool with_reasons = true;
    std::chrono::duration<int, std::milli> initial_retry{1000};

    std::chrono::duration<int, std::milli> connect_timeout{1000};
    std::chrono::duration<int, std::milli> read_timeout{1000};
    Logger logger(std::make_unique<ConsoleBackend>("Test"));

    auto endpoints = config::ServiceEndpoints(
        "", "https://clientstream.launchdarkly.com", "");

    HttpProperties http_properties{
        connect_timeout, read_timeout, "CPP/Client",
        std::map<std::string, std::vector<std::string>>()};

//    ioc.

    auto handler = std::make_shared<Handler>(logger);
    StreamingDataSource ds(ioc.get_executor(), context, use_report, with_reasons, initial_retry,
                           endpoints, http_properties, handler, logger);

    ds.start();
    ioc.run();

    LD_LOG(logger, LogLevel::kInfo) << "SOMETHING";
}
