#include <gtest/gtest.h>

#include "data_sources/data_source_status_manager.hpp"
#include "data_sources/data_source_update_sink.hpp"
#include "data_sources/streaming_data_source.hpp"

#include <launchdarkly/config/shared/builders/endpoints_builder.hpp>
#include <launchdarkly/config/shared/builders/http_properties_builder.hpp>
#include <launchdarkly/config/shared/defaults.hpp>
#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/logging/logger.hpp>
#include <launchdarkly/serialization/json_context.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/write.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

using config::shared::ClientSDK;
using config::shared::Defaults;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace {

using FlagData = std::unordered_map<std::string, ItemDescriptor>;
using Request = http::request<http::string_body>;

Logger MakeNullLogger() {
    struct NullBackend : ILogBackend {
        bool Enabled(LogLevel) noexcept override { return false; }
        void Write(LogLevel, std::string) noexcept override {}
    };
    return Logger{std::make_shared<NullBackend>()};
}

// Records the first flag data that a data source gives to the SDK.
//
// Thread safety: the data source calls Init on its io_context thread.
// WaitForInit is safe from any thread. The recorded data is protected by
// mutex_.
class RecordingSink final : public IDataSourceUpdateSink {
   public:
    void Init(Context const&, FlagData data) override {
        std::lock_guard lock(mutex_);
        if (!init_data_) {
            init_data_ = std::move(data);
        }
        cv_.notify_all();
    }
    void Upsert(Context const&, std::string, ItemDescriptor) override {}
    void Apply(Context const&, FlagChangeSet, bool) override {}

    // Returns the data of the first Init call, or nullopt if Init is not
    // called before the timeout.
    std::optional<FlagData> WaitForInit(std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [this] { return init_data_.has_value(); });
        return init_data_;
    }

   private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::optional<FlagData> init_data_;
};

// Serves a stream that sends a ping event and then stays open, and a polling
// endpoint that returns fixed flag data. Records the first poll request.
//
// Thread safety: the accept and session handlers run on the io_context passed
// to the constructor. WaitForPoll is safe from any thread. The recorded
// request is protected by mutex_.
class PingService {
   public:
    PingService(boost::asio::io_context& ioc, std::string poll_body)
        : acceptor_(ioc,
                    tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),
                                  /* port= */ 0)),
          poll_body_(std::move(poll_body)) {
        Accept();
    }

    [[nodiscard]] std::string Url() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port());
    }

    // Returns the first poll request, or nullopt if no poll request arrives
    // before the timeout.
    std::optional<Request> WaitForPoll(std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [this] { return poll_.has_value(); });
        return poll_;
    }

   private:
    struct Session : std::enable_shared_from_this<Session> {
        Session(tcp::socket socket, PingService* service)
            : socket(std::move(socket)), service(service) {}

        void Run() {
            http::async_read(socket, buffer, request,
                             [self = shared_from_this()](
                                 beast::error_code const& ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 if (std::string(self->request.target())
                                         .rfind("/meval", 0) == 0) {
                                     self->Stream();
                                 } else {
                                     self->Poll();
                                 }
                             });
        }

        // Writes the response head and one ping event, but does not end the
        // response. The stream stays open until the client closes it.
        void Stream() {
            stream_text =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/event-stream\r\n"
                "\r\n"
                "event: ping\n"
                "data: \n"
                "\n";
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

        void Poll() {
            service->RecordPoll(request);
            response.result(http::status::ok);
            response.set(http::field::content_type, "application/json");
            response.body() = service->poll_body_;
            response.keep_alive(false);
            response.prepare_payload();
            http::async_write(
                socket, response,
                [self = shared_from_this()](beast::error_code ec, std::size_t) {
                    self->socket.shutdown(tcp::socket::shutdown_send, ec);
                });
        }

        tcp::socket socket;
        beast::flat_buffer buffer;
        Request request;
        http::response<http::string_body> response;
        std::string stream_text;
        PingService* service;
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

    void RecordPoll(Request const& request) {
        std::lock_guard lock(mutex_);
        if (!poll_) {
            poll_ = request;
        }
        cv_.notify_all();
    }

    tcp::acceptor acceptor_;
    std::string const poll_body_;

    // Protected by mutex_.
    std::mutex mutex_;
    std::condition_variable cv_;
    std::optional<Request> poll_;
};

}  // namespace

class StreamingDataSourcePingTest : public ::testing::Test {
   protected:
    // Starts a streaming data source whose stream sends a ping event. Returns
    // the poll request that the data source sends and the flag data that it
    // gives to the SDK.
    std::pair<std::optional<Request>, std::optional<FlagData>> PingAndWait(
        bool use_report) {
        boost::asio::io_context ioc;
        auto work = boost::asio::make_work_guard(ioc);
        PingService service(ioc, R"({"flagA":{"version":1,"value":"test"}})");

        auto const endpoints =
            config::shared::builders::EndpointsBuilder<ClientSDK>()
                .RelayProxyBaseURL(service.Url())
                .Build();
        config::shared::built::DataSourceConfig<ClientSDK> const
            data_source_config{Defaults<ClientSDK>::StreamingConfig(),
                               /* with_reasons= */ false, use_report};
        auto const http_properties =
            config::shared::builders::HttpPropertiesBuilder<ClientSDK>()
                .Build();

        auto const logger = MakeNullLogger();
        DataSourceStatusManager status_manager;
        RecordingSink sink;
        auto source = std::make_shared<StreamingDataSource>(
            *endpoints, data_source_config, http_properties, ioc.get_executor(),
            context_, sink, status_manager, logger);

        source->Start();
        std::thread thread([&ioc] { ioc.run(); });

        auto poll = service.WaitForPoll(5s);
        auto data = sink.WaitForInit(5s);

        std::promise<void> shutdown;
        boost::asio::post(ioc, [&source, &shutdown] {
            source->ShutdownAsync([&shutdown] { shutdown.set_value(); });
        });
        shutdown.get_future().wait_for(5s);
        work.reset();
        ioc.stop();
        thread.join();
        return {std::move(poll), std::move(data)};
    }

    Context const context_ = ContextBuilder().Kind("user", "user-key").Build();
    FlagData const poll_data_{
        {"flagA", ItemDescriptor(EvaluationResult(
                      1,
                      std::nullopt,
                      false,
                      false,
                      std::nullopt,
                      EvaluationDetailInternal(Value("test"),
                                               std::nullopt,
                                               std::nullopt)))}};
};

TEST_F(StreamingDataSourcePingTest, PingPollsWithGetAndAppliesTheResult) {
    // The stream sends a ping event to a data source that uses GET.
    auto const [poll, data] = PingAndWait(/* use_report= */ false);

    // The data source polls for its context with GET and applies the result.
    ASSERT_TRUE(poll);
    EXPECT_EQ(http::verb::get, poll->method());
    EXPECT_EQ("/msdk/evalx/contexts/" +
                  encoding::Base64UrlEncode(boost::json::serialize(
                      boost::json::value_from(context_))),
              poll->target());
    EXPECT_EQ(poll_data_, data);
}

TEST_F(StreamingDataSourcePingTest, PingPollsWithReportAndAppliesTheResult) {
    // The stream sends a ping event to a data source that uses REPORT.
    auto const [poll, data] = PingAndWait(/* use_report= */ true);

    // The data source polls for its context with REPORT and applies the
    // result.
    ASSERT_TRUE(poll);
    EXPECT_EQ(http::verb::report, poll->method());
    EXPECT_EQ("/msdk/evalx/context", poll->target());
    EXPECT_EQ(boost::json::value_from(context_),
              boost::json::parse(poll->body()));
    EXPECT_EQ(poll_data_, data);
}
