#include <gtest/gtest.h>

#include <launchdarkly/client_side/client.hpp>
#include <launchdarkly/context_builder.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace launchdarkly;
using namespace launchdarkly::client_side;

namespace {

constexpr char kFlagKey[] = "wrong-type-flag";

/* Returns the same flag data for every context. */
class FlagDataPersistence : public IPersistence {
   public:
    explicit FlagDataPersistence(std::string flag_data)
        : flag_data_(std::move(flag_data)) {}

    void Set(std::string, std::string, std::string) noexcept override {}

    void Remove(std::string, std::string) noexcept override {}

    std::optional<std::string> Read(std::string,
                                    std::string key) noexcept override {
        if (key == "ContextIndex") {
            return std::nullopt;
        }
        return flag_data_;
    }

   private:
    std::string flag_data_;
};

/* An HTTP server that keeps the body of each request it gets. */
class EventServer {
   public:
    EventServer()
        : acceptor_(ioc_,
                    tcp::endpoint(net::ip::make_address("127.0.0.1"), 0)) {
        Accept();
        thread_ = std::thread([this]() { ioc_.run(); });
    }

    ~EventServer() {
        ioc_.stop();
        thread_.join();
    }

    std::string Url() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port());
    }

    /* Waits for an event payload and returns its events. */
    std::optional<boost::json::array> NextPayload() {
        std::unique_lock lock(mutex_);
        if (!cv_.wait_for(lock, std::chrono::seconds(5),
                          [this]() { return !bodies_.empty(); })) {
            return std::nullopt;
        }
        auto body = std::move(bodies_.front());
        bodies_.erase(bodies_.begin());
        return boost::json::parse(body).as_array();
    }

   private:
    struct Session : std::enable_shared_from_this<Session> {
        Session(tcp::socket socket, EventServer& server)
            : socket(std::move(socket)), server(server) {}

        void Run() {
            http::async_read(
                socket, buffer, request,
                [self = shared_from_this()](beast::error_code ec,
                                            std::size_t) {
                    if (ec) {
                        return;
                    }
                    self->server.Keep(self->request.body());
                    self->response.result(http::status::accepted);
                    self->response.prepare_payload();
                    http::async_write(
                        self->socket, self->response,
                        [self](beast::error_code, std::size_t) {
                            self->socket.shutdown(
                                tcp::socket::shutdown_both);
                        });
                });
        }

        tcp::socket socket;
        EventServer& server;
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        http::response<http::empty_body> response;
    };

    void Accept() {
        acceptor_.async_accept([this](beast::error_code ec, tcp::socket s) {
            if (!ec) {
                std::make_shared<Session>(std::move(s), *this)->Run();
            }
            Accept();
        });
    }

    void Keep(std::string body) {
        std::lock_guard lock(mutex_);
        bodies_.push_back(std::move(body));
        cv_.notify_all();
    }

    net::io_context ioc_;
    tcp::acceptor acceptor_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::string> bodies_;
};

/* The cached data for a tracked flag that has the string value "a". */
std::string StringFlagData(bool track_reason) {
    return std::string(R"({")") + kFlagKey +
           R"(":{"version":50,"flagVersion":5,"value":"a","variation":0,)"
           R"("reason":{"kind":"FALLTHROUGH"},"trackEvents":true,)"
           R"("trackReason":)" +
           (track_reason ? "true" : "false") + "}}";
}

/* A client that has the flag data, does not connect for flags, and sends
 * its events to the server. */
std::unique_ptr<Client> MakeClient(EventServer const& server,
                                   std::string flag_data) {
    auto builder = ConfigBuilder("sdk-key");
    builder.ServiceEndpoints()
        .PollingBaseUrl(server.Url())
        .StreamingBaseUrl(server.Url())
        .EventsBaseUrl(server.Url());
    builder.Persistence().Custom(
        std::make_shared<FlagDataPersistence>(std::move(flag_data)));
    builder.Logging().Logging(LoggingBuilder::NoLogging());
    return std::make_unique<Client>(
        builder.Build().value(),
        ContextBuilder().Kind("user", "user-key").Build());
}

boost::json::object const* FindEvent(boost::json::array const& events,
                                     std::string const& kind) {
    for (auto const& event : events) {
        if (event.as_object().at("kind").as_string() == kind) {
            return &event.as_object();
        }
    }
    return nullptr;
}

void ExpectWrongTypeEvents(boost::json::array const& events,
                           boost::json::value const& default_value,
                           bool with_reason) {
    auto const* feature = FindEvent(events, "feature");
    ASSERT_NE(feature, nullptr) << boost::json::serialize(events);
    EXPECT_EQ(feature->at("key"), kFlagKey);
    EXPECT_EQ(feature->at("version"), 5);
    EXPECT_EQ(feature->at("value"), default_value);
    EXPECT_EQ(feature->at("default"), default_value);
    EXPECT_FALSE(feature->contains("variation"));
    if (with_reason) {
        EXPECT_EQ(feature->at("reason"),
                  boost::json::parse(
                      R"({"kind":"ERROR","errorKind":"WRONG_TYPE"})"));
    } else {
        EXPECT_FALSE(feature->contains("reason"));
    }

    auto const* summary = FindEvent(events, "summary");
    ASSERT_NE(summary, nullptr) << boost::json::serialize(events);
    auto const& counters =
        summary->at("features").at(kFlagKey).at("counters").as_array();
    ASSERT_EQ(counters.size(), 1);
    auto const& counter = counters[0].as_object();
    EXPECT_EQ(counter.at("value"), default_value);
    EXPECT_EQ(counter.at("version"), 5);
    EXPECT_EQ(counter.at("count"), 1);
    EXPECT_FALSE(counter.contains("variation"));
    EXPECT_FALSE(counter.contains("unknown"));
}

}  // namespace

TEST(WrongTypeEventsTest, EventHasTheDefaultValueAndTheFlagVersion) {
    EventServer server;
    auto client = MakeClient(server, StringFlagData(false));

    EXPECT_TRUE(client->BoolVariation(kFlagKey, true));
    client->FlushAsync();

    auto events = server.NextPayload();
    ASSERT_TRUE(events);
    ExpectWrongTypeEvents(*events, true, false);
}

TEST(WrongTypeEventsTest, DetailCallEventHasTheWrongTypeReason) {
    EventServer server;
    auto client = MakeClient(server, StringFlagData(false));

    auto detail = client->IntVariationDetail(kFlagKey, 3);
    EXPECT_EQ(detail.Value(), 3);
    EXPECT_EQ(detail.Reason(),
              EvaluationReason(EvaluationReason::ErrorKind::kWrongType));
    client->FlushAsync();

    auto events = server.NextPayload();
    ASSERT_TRUE(events);
    // The SDK stores every number as a double.
    ExpectWrongTypeEvents(*events, 3.0, true);
}

TEST(WrongTypeEventsTest, EventHasTheWrongTypeReasonWhenTheFlagTracksReasons) {
    EventServer server;
    auto client = MakeClient(server, StringFlagData(true));

    EXPECT_EQ(client->DoubleVariation(kFlagKey, 2.5), 2.5);
    client->FlushAsync();

    auto events = server.NextPayload();
    ASSERT_TRUE(events);
    ExpectWrongTypeEvents(*events, 2.5, true);
}
