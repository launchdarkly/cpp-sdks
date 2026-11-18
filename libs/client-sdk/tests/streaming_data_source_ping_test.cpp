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
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace launchdarkly;
using namespace launchdarkly::client_side;
using namespace launchdarkly::client_side::data_sources;
using namespace std::chrono_literals;

using config::shared::ClientSDK;
using config::shared::Defaults;
using DataSourceState = DataSourceStatus::DataSourceState;
using ErrorKind = DataSourceStatus::ErrorInfo::ErrorKind;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace {

using FlagData = std::unordered_map<std::string, ItemDescriptor>;
using Request = http::request<http::string_body>;

char const* const kPingEvent = "event: ping\ndata: \n\n";

// Returns a poll response body with one flag, "flagA".
std::string FlagsJson(int version, std::string const& value) {
    return R"({"flagA":{"version":)" + std::to_string(version) +
           R"(,"value":")" + value + R"("}})";
}

// Returns the value of "flagA" in each data set.
std::vector<std::string> FlagAValues(std::vector<FlagData> const& data_sets) {
    std::vector<std::string> values;
    for (auto const& data : data_sets) {
        values.push_back(data.at("flagA").item->Detail().Value().AsString());
    }
    return values;
}

// Records the messages that the data source logs.
//
// Thread safety: Write is safe from any thread, as ILogBackend requires.
// WaitForMessage is safe from any thread. The messages are protected by
// mutex_.
class RecordingLogBackend final : public ILogBackend {
   public:
    bool Enabled(LogLevel) noexcept override { return true; }

    void Write(LogLevel, std::string message) noexcept override {
        std::lock_guard lock(mutex_);
        messages_.push_back(std::move(message));
        cv_.notify_all();
    }

    // Returns true if a message that contains the text is logged before the
    // timeout.
    bool WaitForMessage(std::string const& text,
                        std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] {
            for (auto const& message : messages_) {
                if (message.find(text) != std::string::npos) {
                    return true;
                }
            }
            return false;
        });
    }

   private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> messages_;
};

// Records the flag data that a data source gives to the SDK.
//
// Thread safety: the data source calls Init and Upsert on its executor
// thread. The Wait methods are safe from any thread. The recorded data is
// protected by mutex_.
class RecordingSink final : public IDataSourceUpdateSink {
   public:
    void Init(Context const&, FlagData data) override {
        std::lock_guard lock(mutex_);
        inits_.push_back(std::move(data));
        cv_.notify_all();
    }

    void Upsert(Context const&, std::string key, ItemDescriptor) override {
        std::lock_guard lock(mutex_);
        upserts_.push_back(std::move(key));
        cv_.notify_all();
    }

    void Apply(Context const&, FlagChangeSet, bool) override {}

    // Waits until Init is called at least count times, or until the timeout.
    // Returns the data of every Init call.
    std::vector<FlagData> WaitForInits(std::size_t count,
                                       std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [&] { return inits_.size() >= count; });
        return inits_;
    }

    // Waits until Upsert is called at least count times, or until the
    // timeout. Returns the key of every Upsert call.
    std::vector<std::string> WaitForUpserts(std::size_t count,
                                            std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        cv_.wait_for(lock, timeout, [&] { return upserts_.size() >= count; });
        return upserts_;
    }

   private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<FlagData> inits_;
    std::vector<std::string> upserts_;
};

// Serves a stream and a polling endpoint from its own thread, as the Relay
// Proxy does for a ping stream. The stream sends a ping when a client
// connects, and then the events that the test sends. The test answers each
// poll request.
//
// Thread safety: the public methods are safe from any thread. The network
// handlers run on the thread of the service, and only they use sessions_,
// polls_ and stream_. The recorded poll requests are protected by mutex_.
class PingService {
   public:
    PingService()
        : acceptor_(ioc_,
                    tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),
                                  /* port= */ 0)),
          url_("http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port())) {
        Accept();
        thread_ = std::thread([this] { ioc_.run(); });
    }

    // Closes every connection. Then the handlers end and the thread exits.
    ~PingService() {
        boost::asio::post(ioc_, [this] {
            beast::error_code ec;
            acceptor_.close(ec);
            for (auto const& session : sessions_) {
                session->socket.close(ec);
            }
        });
        thread_.join();
    }

    [[nodiscard]] std::string const& Url() const { return url_; }

    // Sends an event on the stream. The stream must be open.
    void Send(std::string event) {
        boost::asio::post(ioc_, [this, event = std::move(event)]() mutable {
            Write(stream_, std::move(event));
        });
    }

    // Returns poll request number index (from 0), or nullopt if it does not
    // arrive before the timeout.
    std::optional<Request> WaitForPoll(std::size_t index,
                                       std::chrono::milliseconds timeout) {
        std::unique_lock lock(mutex_);
        if (!cv_.wait_for(lock, timeout,
                          [&] { return poll_requests_.size() > index; })) {
            return std::nullopt;
        }
        return poll_requests_[index];
    }

    [[nodiscard]] std::size_t PollCount() {
        std::lock_guard lock(mutex_);
        return poll_requests_.size();
    }

    // Answers poll request number index, which must have arrived.
    void RespondToPoll(std::size_t index,
                       http::status status,
                       std::string body) {
        boost::asio::post(
            ioc_, [this, index, status, body = std::move(body)]() mutable {
                auto session = polls_.at(index);
                session->response.result(status);
                session->response.set(http::field::content_type,
                                      "application/json");
                session->response.body() = std::move(body);
                session->response.keep_alive(false);
                session->response.prepare_payload();
                http::async_write(session->socket, session->response,
                                  [session](beast::error_code ec, std::size_t) {
                                      session->socket.shutdown(
                                          tcp::socket::shutdown_send, ec);
                                  });
            });
    }

    // Closes the connection of poll request number index without a response.
    // The poll request must have arrived.
    void ClosePoll(std::size_t index) {
        boost::asio::post(ioc_, [this, index] {
            beast::error_code ec;
            polls_.at(index)->socket.close(ec);
        });
    }

   private:
    struct Session {
        explicit Session(tcp::socket socket) : socket(std::move(socket)) {}

        tcp::socket socket;
        beast::flat_buffer buffer;
        Request request;
        http::response<http::string_body> response;
        std::deque<std::string> writes;
    };

    void Accept() {
        acceptor_.async_accept(
            [this](beast::error_code const& ec, tcp::socket socket) {
                if (ec) {
                    return;
                }
                auto session = std::make_shared<Session>(std::move(socket));
                sessions_.push_back(session);
                Read(session);
                Accept();
            });
    }

    void Read(std::shared_ptr<Session> const& session) {
        http::async_read(
            session->socket, session->buffer, session->request,
            [this, session](beast::error_code const& ec, std::size_t) {
                if (ec) {
                    return;
                }
                if (std::string(session->request.target()).rfind("/meval", 0) ==
                    0) {
                    stream_ = session;
                    Write(session,
                          std::string("HTTP/1.1 200 OK\r\n"
                                      "Content-Type: text/event-stream\r\n"
                                      "\r\n") +
                              kPingEvent);
                    return;
                }
                polls_.push_back(session);
                std::lock_guard lock(mutex_);
                poll_requests_.push_back(session->request);
                cv_.notify_all();
            });
    }

    // Writes the text after any earlier writes on the same connection.
    void Write(std::shared_ptr<Session> const& session, std::string text) {
        session->writes.push_back(std::move(text));
        if (session->writes.size() == 1) {
            WriteNext(session);
        }
    }

    void WriteNext(std::shared_ptr<Session> const& session) {
        boost::asio::async_write(
            session->socket, boost::asio::buffer(session->writes.front()),
            [this, session](beast::error_code const& ec, std::size_t) {
                session->writes.pop_front();
                if (ec) {
                    session->writes.clear();
                    return;
                }
                if (!session->writes.empty()) {
                    WriteNext(session);
                }
            });
    }

    boost::asio::io_context ioc_;
    tcp::acceptor acceptor_;
    std::string const url_;
    std::thread thread_;

    // Used only on the thread of the service.
    std::vector<std::shared_ptr<Session>> sessions_;
    std::vector<std::shared_ptr<Session>> polls_;
    std::shared_ptr<Session> stream_;

    // Protected by mutex_.
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Request> poll_requests_;
};

}  // namespace

class StreamingDataSourcePingTest : public ::testing::Test {
   protected:
    void SetUp() override {
        status_connection_ = status_manager_.OnDataSourceStatusChange(
            [this](DataSourceStatus status) {
                std::lock_guard lock(status_mutex_);
                latest_status_.emplace(std::move(status));
                status_cv_.notify_all();
            });
    }

    void TearDown() override {
        if (source_ && !shut_down_) {
            Shutdown();
        }
        status_connection_->Disconnect();
        work_.reset();
        ioc_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    // Starts a data source that streams from the service. The service sends
    // a ping as soon as the data source connects.
    void Start(bool use_report, bool with_reasons) {
        auto const endpoints =
            config::shared::builders::EndpointsBuilder<ClientSDK>()
                .RelayProxyBaseURL(service_.Url())
                .Build();
        config::shared::built::DataSourceConfig<ClientSDK> const
            data_source_config{Defaults<ClientSDK>::StreamingConfig(),
                               with_reasons, use_report};
        auto const http_properties =
            config::shared::builders::HttpPropertiesBuilder<ClientSDK>()
                .Build();
        source_ = std::make_shared<StreamingDataSource>(
            *endpoints, data_source_config, http_properties,
            ioc_.get_executor(), context_, sink_, status_manager_, logger_);
        source_->Start();
        thread_ = std::thread([this] { ioc_.run(); });
    }

    // Shuts the data source down and waits for the shutdown to complete.
    void Shutdown() {
        auto done = std::make_shared<std::promise<void>>();
        boost::asio::post(ioc_, [this, done] {
            source_->ShutdownAsync([done] { done->set_value(); });
        });
        shut_down_ = true;
        ASSERT_EQ(std::future_status::ready, done->get_future().wait_for(5s));
    }

    // Returns the status once its state is the given state, or nullopt if
    // that does not happen before the timeout.
    std::optional<DataSourceStatus> WaitForState(DataSourceState state) {
        std::unique_lock lock(status_mutex_);
        if (!status_cv_.wait_for(lock, 5s, [&] {
                return latest_status_ && latest_status_->State() == state;
            })) {
            return std::nullopt;
        }
        return latest_status_;
    }

    [[nodiscard]] std::string EncodedContext() const {
        return encoding::Base64UrlEncode(
            boost::json::serialize(boost::json::value_from(context_)));
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

    PingService service_;
    std::shared_ptr<RecordingLogBackend> const log_ =
        std::make_shared<RecordingLogBackend>();
    Logger const logger_{log_};
    RecordingSink sink_;
    DataSourceStatusManager status_manager_;
    std::unique_ptr<IConnection> status_connection_;

    // Protected by status_mutex_.
    std::mutex status_mutex_;
    std::condition_variable status_cv_;
    std::optional<DataSourceStatus> latest_status_;

    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        work_ = boost::asio::make_work_guard(ioc_);
    std::shared_ptr<StreamingDataSource> source_;
    bool shut_down_ = false;
    std::thread thread_;
};

TEST_F(StreamingDataSourcePingTest, PingPollsWithGetAndAppliesTheResult) {
    // The stream sends a ping event to a data source that uses GET.
    Start(/* use_report= */ false, /* with_reasons= */ false);
    auto const poll = service_.WaitForPoll(0, 5s);
    ASSERT_TRUE(poll);
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "test"));

    // The data source polls for its context with GET and applies the result.
    EXPECT_EQ(http::verb::get, poll->method());
    EXPECT_EQ("/msdk/evalx/contexts/" + EncodedContext(), poll->target());
    EXPECT_EQ(std::vector<FlagData>{poll_data_}, sink_.WaitForInits(1, 5s));
    EXPECT_TRUE(WaitForState(DataSourceState::kValid));
}

TEST_F(StreamingDataSourcePingTest, PingPollsWithReportAndAppliesTheResult) {
    // The stream sends a ping event to a data source that uses REPORT.
    Start(/* use_report= */ true, /* with_reasons= */ false);
    auto const poll = service_.WaitForPoll(0, 5s);
    ASSERT_TRUE(poll);
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "test"));

    // The data source polls for its context with REPORT and applies the
    // result.
    EXPECT_EQ(http::verb::report, poll->method());
    EXPECT_EQ("/msdk/evalx/context", poll->target());
    EXPECT_EQ(boost::json::value_from(context_),
              boost::json::parse(poll->body()));
    EXPECT_EQ(std::vector<FlagData>{poll_data_}, sink_.WaitForInits(1, 5s));
}

TEST_F(StreamingDataSourcePingTest, PingPollRequestsReasonsWhenConfigured) {
    Start(/* use_report= */ false, /* with_reasons= */ true);

    auto const poll = service_.WaitForPoll(0, 5s);
    ASSERT_TRUE(poll);
    EXPECT_EQ("/msdk/evalx/contexts/" + EncodedContext() + "?withReasons=true",
              poll->target());
}

TEST_F(StreamingDataSourcePingTest, PingsDuringAPollCauseOneMorePoll) {
    Start(/* use_report= */ false, /* with_reasons= */ false);
    ASSERT_TRUE(service_.WaitForPoll(0, 5s));

    // Three pings arrive while the first poll is in flight. The patch after
    // them shows that the data source has handled them.
    service_.Send(kPingEvent);
    service_.Send(kPingEvent);
    service_.Send(kPingEvent);
    service_.Send(
        "event: patch\n"
        R"(data: {"key": "flagB", "version": 1, "value": true})"
        "\n\n");
    ASSERT_EQ(1u, sink_.WaitForUpserts(1, 5s).size());

    // The data source applies the first poll, then polls one more time and
    // applies that result last.
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "old"));
    ASSERT_TRUE(service_.WaitForPoll(1, 5s));
    service_.RespondToPoll(1, http::status::ok, FlagsJson(2, "new"));
    EXPECT_EQ((std::vector<std::string>{"old", "new"}),
              FlagAValues(sink_.WaitForInits(2, 5s)));

    // The next ping polls again. The three pings caused only one poll.
    service_.Send(kPingEvent);
    ASSERT_TRUE(service_.WaitForPoll(2, 5s));
    EXPECT_EQ(3u, service_.PollCount());
}

TEST_F(StreamingDataSourcePingTest,
       ErrorStatusInterruptsAndNextPingPollsAgain) {
    Start(/* use_report= */ false, /* with_reasons= */ false);
    ASSERT_TRUE(service_.WaitForPoll(0, 5s));
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "old"));
    ASSERT_TRUE(WaitForState(DataSourceState::kValid));

    // The poll after the next ping gets a server error.
    service_.Send(kPingEvent);
    ASSERT_TRUE(service_.WaitForPoll(1, 5s));
    service_.RespondToPoll(1, http::status::service_unavailable, "");

    // The data source reports the error and stays connected.
    auto const interrupted = WaitForState(DataSourceState::kInterrupted);
    ASSERT_TRUE(interrupted);
    ASSERT_TRUE(interrupted->LastError());
    EXPECT_EQ(ErrorKind::kErrorResponse, interrupted->LastError()->Kind());
    EXPECT_EQ(503, interrupted->LastError()->StatusCode());
    EXPECT_EQ(
        "HTTP error 503 for: polling request after a ping - will poll again on "
        "the next ping",
        interrupted->LastError()->Message());

    // The next ping polls again and applies the result.
    service_.Send(kPingEvent);
    ASSERT_TRUE(service_.WaitForPoll(2, 5s));
    service_.RespondToPoll(2, http::status::ok, FlagsJson(2, "new"));
    EXPECT_TRUE(WaitForState(DataSourceState::kValid));
    EXPECT_EQ((std::vector<std::string>{"old", "new"}),
              FlagAValues(sink_.WaitForInits(2, 5s)));
}

TEST_F(StreamingDataSourcePingTest, NetworkErrorOnPollInterrupts) {
    Start(/* use_report= */ false, /* with_reasons= */ false);
    ASSERT_TRUE(service_.WaitForPoll(0, 5s));
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "old"));
    ASSERT_TRUE(WaitForState(DataSourceState::kValid));

    // The connection of the poll after the next ping closes with no response.
    service_.Send(kPingEvent);
    ASSERT_TRUE(service_.WaitForPoll(1, 5s));
    service_.ClosePoll(1);

    auto const interrupted = WaitForState(DataSourceState::kInterrupted);
    ASSERT_TRUE(interrupted);
    ASSERT_TRUE(interrupted->LastError());
    EXPECT_EQ(ErrorKind::kNetworkError, interrupted->LastError()->Kind());
}

TEST_F(StreamingDataSourcePingTest, PollThatCompletesAfterShutdownIsDiscarded) {
    Start(/* use_report= */ false, /* with_reasons= */ false);
    ASSERT_TRUE(service_.WaitForPoll(0, 5s));
    Shutdown();

    // The poll completes after the data source shuts down.
    service_.RespondToPoll(0, http::status::ok, FlagsJson(1, "stale"));

    // The data source discards the result.
    ASSERT_TRUE(log_->WaitForMessage(
        "Discarding the result of a poll that completed after shutdown", 5s));
    EXPECT_TRUE(sink_.WaitForInits(0, 0ms).empty());
    EXPECT_EQ(DataSourceState::kInitializing, status_manager_.Status().State());
}
