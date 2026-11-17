#include <gtest/gtest.h>

#include "data_sources/data_source_status_manager.hpp"
#include "data_sources/data_source_update_sink.hpp"
#include "data_sources/streaming_data_source.hpp"

#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

using config::shared::ClientSDK;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace {

Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

class NoopSink final : public IDataSourceUpdateSink {
   public:
    void Init(Context const&,
              std::unordered_map<std::string, ItemDescriptor>) override {}
    void Upsert(Context const&, std::string, ItemDescriptor) override {}
    void Apply(Context const&, FlagChangeSet, bool) override {}
};

// Serves a stream that sends a fixed event on the first connection. Every
// connection stays open until the client closes it. Counts the connections.
//
// Thread safety: the accept and session handlers run on the io_context passed
// to the constructor. WaitForConnections is safe from any thread. The
// connection count is protected by mutex_.
class StreamService {
   public:
    StreamService(boost::asio::io_context& ioc, std::string first_event)
        : acceptor_(ioc,
                    tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),
                                  /* port= */ 0)),
          first_event_(std::move(first_event)) {
        Accept();
    }

    [[nodiscard]] std::string Url() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port());
    }

    // Returns true when the service has received at least count stream
    // connections before the timeout.
    bool WaitForConnections(int count, std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout,
                            [this, count] { return connections_ >= count; });
    }

   private:
    struct Session : std::enable_shared_from_this<Session> {
        Session(tcp::socket socket, StreamService* service)
            : socket(std::move(socket)), service(service) {}

        void Run() {
            http::async_read(socket, buffer, request,
                             [self = shared_from_this()](
                                 beast::error_code const& ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->Stream(self->service->CountConnection());
                             });
        }

        // Writes the response head, and the event if this is the first
        // connection, but does not end the response.
        void Stream(int connection) {
            stream_text =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "\r\n";
            if (connection == 1) {
                stream_text += service->first_event_;
            }
            boost::asio::async_write(
                socket, boost::asio::buffer(stream_text),
                [self = shared_from_this()](beast::error_code const& ec,
                                            std::size_t) {
                    if (ec) {
                        return;
                    }
                    self->socket.async_wait(
                        tcp::socket::wait_read,
                        [self](beast::error_code const&) {});
                });
        }

        tcp::socket socket;
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        std::string stream_text;
        StreamService* service;
    };

    void Accept() {
        acceptor_.async_accept(
            [this](beast::error_code const& ec, tcp::socket socket) {
                if (ec) {
                    return;
                }
                std::make_shared<Session>(std::move(socket), this)->Run();
                Accept();
            });
    }

    // Returns the number of connections, this one included.
    int CountConnection() {
        std::lock_guard lock(mutex_);
        ++connections_;
        cv_.notify_all();
        return connections_;
    }

    tcp::acceptor acceptor_;
    std::string const first_event_;

    // Protected by mutex_.
    std::mutex mutex_;
    std::condition_variable cv_;
    int connections_ = 0;
};

struct InvalidEvent {
    std::string name;
    std::string event;
};

}  // namespace

class StreamingDataSourceInvalidDataTest
    : public ::testing::TestWithParam<InvalidEvent> {};

TEST_P(StreamingDataSourceInvalidDataTest, RestartsTheStream) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);
    StreamService service(ioc, GetParam().event);

    auto const endpoints =
        config::shared::builders::EndpointsBuilder<ClientSDK>()
            .RelayProxyBaseURL(service.Url())
            .Build();
    ASSERT_TRUE(endpoints);
    config::shared::built::DataSourceConfig<ClientSDK> const data_source_config{
        config::shared::built::StreamingConfig<ClientSDK>{
            /* initial_reconnect_delay= */ 1ms, /* streaming_path= */ "/meval"},
        /* with_reasons= */ false, /* use_report= */ false};
    auto const http_properties =
        config::shared::builders::HttpPropertiesBuilder<ClientSDK>().Build();

    auto const logger = MakeNullLogger();
    DataSourceStatusManager status_manager;
    NoopSink sink;
    auto source = std::make_shared<StreamingDataSource>(
        *endpoints, data_source_config, http_properties, ioc.get_executor(),
        ContextBuilder().Kind("user", "user-key").Build(), sink, status_manager,
        logger);

    // Start the data source. The stream sends the invalid event and stays
    // open.
    source->Start();
    std::thread thread([&ioc] { ioc.run(); });

    // The data source must close the stream and open a new one.
    EXPECT_TRUE(service.WaitForConnections(2, 5s));

    std::promise<void> shutdown;
    boost::asio::post(ioc, [&source, &shutdown] {
        source->ShutdownAsync([&shutdown] { shutdown.set_value(); });
    });
    shutdown.get_future().wait_for(5s);
    work.reset();
    ioc.stop();
    thread.join();
}

INSTANTIATE_TEST_SUITE_P(
    InvalidEvents,
    StreamingDataSourceInvalidDataTest,
    ::testing::Values(
        InvalidEvent{"MalformedPut", "event: put\ndata: {sorry\n\n"},
        InvalidEvent{"MalformedPatch", "event: patch\ndata: {sorry\n\n"},
        InvalidEvent{"MalformedDelete", "event: delete\ndata: {sorry\n\n"},
        InvalidEvent{"SchemaMismatchPut",
                     "event: put\ndata: {\"potato\": {}}\n\n"},
        InvalidEvent{"SchemaMismatchPatch",
                     "event: patch\ndata: {\"potato\": {}}\n\n"},
        InvalidEvent{"SchemaMismatchDelete",
                     "event: delete\ndata: {\"potato\": {}}\n\n"}),
    [](::testing::TestParamInfo<InvalidEvent> const& info) {
        return info.param.name;
    });
