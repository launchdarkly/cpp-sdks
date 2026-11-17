#include <gtest/gtest.h>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace launchdarkly;
using namespace launchdarkly::server_side;

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

namespace {

// Answers every request with HTTP 202 and records the request bodies.
//
// Thread safety: the accept and session handlers run on the io_context passed
// to the constructor. Bodies is safe from any thread. The recorded bodies are
// protected by mutex_.
class EventsService {
   public:
    explicit EventsService(boost::asio::io_context& ioc)
        : acceptor_(ioc,
                    tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"),
                                  /* port= */ 0)) {
        Accept();
    }

    [[nodiscard]] std::string Url() const {
        return "http://127.0.0.1:" +
               std::to_string(acceptor_.local_endpoint().port());
    }

    [[nodiscard]] std::vector<std::string> Bodies() {
        std::lock_guard lock(mutex_);
        return bodies_;
    }

   private:
    struct Session : std::enable_shared_from_this<Session> {
        Session(tcp::socket socket, EventsService* service)
            : socket(std::move(socket)), service(service) {}

        void Run() {
            http::async_read(socket, buffer, request,
                             [self = shared_from_this()](
                                 beast::error_code const& ec, std::size_t) {
                                 if (ec) {
                                     return;
                                 }
                                 self->service->Record(self->request.body());
                                 self->response.result(http::status::accepted);
                                 self->response.keep_alive(false);
                                 self->response.prepare_payload();
                                 http::async_write(
                                     self->socket, self->response,
                                     [self](beast::error_code ec, std::size_t) {
                                         self->socket.shutdown(
                                             tcp::socket::shutdown_send, ec);
                                     });
                             });
        }

        tcp::socket socket;
        beast::flat_buffer buffer;
        http::request<http::string_body> request;
        http::response<http::empty_body> response;
        EventsService* service;
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

    void Record(std::string body) {
        std::lock_guard lock(mutex_);
        bodies_.push_back(std::move(body));
    }

    tcp::acceptor acceptor_;

    // Protected by mutex_.
    std::mutex mutex_;
    std::vector<std::string> bodies_;
};

}  // namespace

TEST(ClientCloseTest, DestroyingTheClientDeliversPendingEvents) {
    boost::asio::io_context ioc;
    auto work = boost::asio::make_work_guard(ioc);
    EventsService service(ioc);
    std::thread thread([&ioc] { ioc.run(); });

    ConfigBuilder builder("sdk-key");
    builder.ServiceEndpoints().RelayProxyBaseURL(service.Url());
    builder.Events().FlushInterval(std::chrono::hours(1));
    builder.DataSystem().Disable();
    auto config = builder.Build();
    ASSERT_TRUE(config);

    // Record a custom event, then destroy the client before the flush
    // interval ends.
    {
        Client client(std::move(*config));
        client.Track(ContextBuilder().Kind("user", "user-key").Build(),
                     "close-test-event");
    }

    // The client delivered the pending event before its destruction ended.
    auto const bodies = service.Bodies();
    work.reset();
    ioc.stop();
    thread.join();
    ASSERT_EQ(1, bodies.size());
    EXPECT_NE(std::string::npos, bodies[0].find("close-test-event"));
}
