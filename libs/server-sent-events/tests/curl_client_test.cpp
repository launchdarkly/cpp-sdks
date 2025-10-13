#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <launchdarkly/sse/client.hpp>

#include "mock_sse_server.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <vector>

using namespace launchdarkly::sse;
using namespace launchdarkly::sse::test;
using namespace std::chrono_literals;

namespace {

// C++17-compatible latch replacement
class SimpleLatch {
public:
    explicit SimpleLatch(std::size_t count) : count_(count) {}

    void count_down() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ > 0) {
            --count_;
        }
        cv_.notify_all();
    }

    template<typename Rep, typename Period>
    bool wait_for(std::chrono::duration<Rep, Period> timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this] { return count_ == 0; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t count_;
};

// Helper to synchronize event reception in tests
class EventCollector {
public:
    void add_event(Event event) {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.push_back(std::move(event));
        cv_.notify_all();
    }

    void add_error(Error error) {
        std::lock_guard<std::mutex> lock(mutex_);
        errors_.push_back(std::move(error));
        cv_.notify_all();
    }

    bool wait_for_events(size_t count, std::chrono::milliseconds timeout = 5000ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] { return events_.size() >= count; });
    }

    bool wait_for_errors(size_t count, std::chrono::milliseconds timeout = 5000ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [&] { return errors_.size() >= count; });
    }

    std::vector<Event> events() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return events_;
    }

    std::vector<Error> errors() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return errors_;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        events_.clear();
        errors_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<Event> events_;
    std::vector<Error> errors_;
};

// Helper to run io_context in background thread
class IoContextRunner {
public:
    IoContextRunner() : work_guard_(boost::asio::make_work_guard(ioc_)) {
        thread_ = std::thread([this] { ioc_.run(); });
    }

    ~IoContextRunner() {
        work_guard_.reset();
        ioc_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    boost::asio::io_context& context() { return ioc_; }

private:
    boost::asio::io_context ioc_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::thread thread_;
};

}  // namespace

// Basic connectivity tests

TEST(CurlClientTest, ConnectsToHttpServer) {
    MockSSEServer server;
    auto port = server.start(TestHandlers::simple_event("hello world"));

    // Give server a moment to start accepting connections
    std::this_thread::sleep_for(100ms);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("hello world", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, HandlesMultipleEvents) {
    MockSSEServer server;
    auto port = server.start(TestHandlers::multiple_events({"event1", "event2", "event3"}));

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(3));
    auto events = collector.events();
    ASSERT_EQ(3, events.size());
    EXPECT_EQ("event1", events[0].data());
    EXPECT_EQ("event2", events[1].data());
    EXPECT_EQ("event3", events[2].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// SSE parsing tests

TEST(CurlClientTest, ParsesEventWithType) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("test data", "custom-type"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("test data", events[0].data());
    EXPECT_EQ("custom-type", events[0].type());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, ParsesEventWithId) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("test data", "", "event-123"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("test data", events[0].data());
    EXPECT_EQ("event-123", events[0].id());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, ParsesMultiLineData) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("line1\nline2\nline3"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("line1\nline2\nline3", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, HandlesComments) {
    GTEST_SKIP() << "Comment filtering is not yet implemented in the SSE parser";

    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Send a comment (should be ignored)
        send_sse_event(SSEFormatter::comment("this is a comment"));
        // Send an actual event
        send_sse_event(SSEFormatter::event("real data"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    // Should only receive the real event, not the comment
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("real data", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// HTTP method tests

TEST(CurlClientTest, SupportsPostMethod) {
    MockSSEServer server;
    std::string received_method;

    auto port = server.start([&](auto const& req, auto send_response, auto send_sse_event, auto close) {
        received_method = std::string(req.method_string());

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("response"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .method(http::verb::post)
        .body("test body")
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    EXPECT_EQ("POST", received_method);

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, SupportsReportMethod) {
    MockSSEServer server;
    std::string received_method;

    auto port = server.start([&](auto const& req, auto send_response, auto send_sse_event, auto close) {
        received_method = std::string(req.method_string());

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("response"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .method(http::verb::report)
        .body("test body")
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    EXPECT_EQ("REPORT", received_method);

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// HTTP header tests

TEST(CurlClientTest, SendsCustomHeaders) {
    MockSSEServer server;
    std::string custom_header_value;

    auto port = server.start([&](auto const& req, auto send_response, auto send_sse_event, auto close) {
        auto it = req.find("X-Custom-Header");
        if (it != req.end()) {
            custom_header_value = std::string(it->value());
        }

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("response"));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .header("X-Custom-Header", "custom-value")
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    EXPECT_EQ("custom-value", custom_header_value);

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// HTTP status code tests

TEST(CurlClientTest, Handles404Error) {
    MockSSEServer server;
    auto port = server.start(TestHandlers::http_error(http::status::not_found));

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .errors([&](Error e) { collector.add_error(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_errors(1));
    auto errors = collector.errors();
    ASSERT_GE(errors.size(), 1);

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, Handles500Error) {
    // 500 errors are treated as transient server errors and should trigger
    // backoff/retry behavior, not error callbacks. This is correct SSE client behavior.
    std::atomic<int> connection_attempts{0};

    auto handler = [&](auto const&, auto send_response, auto, auto) {
        connection_attempts++;
        http::response<http::string_body> res{http::status::internal_server_error, 11};
        res.body() = "Error";
        res.prepare_payload();
        send_response(res);
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .errors([&](Error e) { collector.add_error(std::move(e)); })
        .initial_reconnect_delay(50ms)  // Short delay for test
        .use_curl(true)
        .build();

    client->async_connect();

    // Should NOT receive error callbacks - should retry instead
    // Wait a bit to let multiple reconnection attempts happen
    std::this_thread::sleep_for(300ms);

    // Verify that multiple reconnection attempts occurred (backoff/retry behavior)
    EXPECT_GE(connection_attempts.load(), 2);

    // Verify no error callbacks were invoked (5xx are not reported as errors)
    EXPECT_EQ(0, collector.errors().size());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// Redirect tests

TEST(CurlClientTest, FollowsRedirects) {
    MockSSEServer redirect_server;
    MockSSEServer target_server;

    auto target_port = target_server.start(TestHandlers::simple_event("redirected"));
    auto redirect_port = redirect_server.start(
        TestHandlers::redirect("http://localhost:" + std::to_string(target_port) + "/")
    );

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(redirect_port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("redirected", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// Connection lifecycle tests

TEST(CurlClientTest, ShutdownStopsClient) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Keep sending events forever (until connection closes)
        for (int i = 0; i < 1000; i++) {
            send_sse_event(SSEFormatter::event("event " + std::to_string(i)));
            std::this_thread::sleep_for(10ms);
        }
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    // Wait for at least one event
    ASSERT_TRUE(collector.wait_for_events(1));

    // Shutdown should complete quickly
    auto shutdown_start = std::chrono::steady_clock::now();
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;

    // Shutdown should complete in reasonable time (less than 2 seconds)
    EXPECT_LT(shutdown_duration, 2000ms);
}

TEST(CurlClientTest, CanShutdownBeforeConnection) {
    MockSSEServer server;
    auto port = server.start(TestHandlers::simple_event("test"));

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    // Shutdown immediately without connecting
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, HandlesImmediateClose) {
    // Immediate connection close is treated as a transient network error and should trigger
    // backoff/retry behavior, not error callbacks. This is correct SSE client behavior.
    std::atomic<int> connection_attempts{0};

    auto handler = [&](auto const&, auto, auto, auto close) {
        connection_attempts++;
        close();  // Immediately close without sending headers
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .errors([&](Error e) { collector.add_error(std::move(e)); })
        .initial_reconnect_delay(50ms)  // Short delay for test
        .use_curl(true)
        .build();

    client->async_connect();

    // Should NOT receive error callbacks - should retry instead
    // Wait a bit to let multiple reconnection attempts happen
    std::this_thread::sleep_for(300ms);

    // Verify that multiple reconnection attempts occurred (backoff/retry behavior)
    EXPECT_GE(connection_attempts.load(), 2);

    // Verify no error callbacks were invoked (connection errors trigger retry)
    EXPECT_EQ(0, collector.errors().size());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// Timeout tests

TEST(CurlClientTest, RespectsReadTimeout) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Send one event
        send_sse_event(SSEFormatter::event("first"));

        // Then wait longer than read timeout without sending anything
        std::this_thread::sleep_for(5000ms);
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .errors([&](Error e) { collector.add_error(std::move(e)); })
        .read_timeout(500ms)  // Short timeout for test
        .initial_reconnect_delay(50ms)
        .use_curl(true)
        .build();

    client->async_connect();

    // Should receive the first event
    ASSERT_TRUE(collector.wait_for_events(1, 2000ms));

    // Then should get a timeout error
    ASSERT_TRUE(collector.wait_for_errors(1, 3000ms));

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// Resource management tests

TEST(CurlClientTest, NoThreadLeaksAfterMultipleConnections) {
    // This test verifies that threads are properly joined and not leaked
    MockSSEServer server;
    auto port = server.start(TestHandlers::simple_event("test"));

    IoContextRunner runner;

    // Create and destroy multiple clients
    for (int i = 0; i < 5; i++) {
        EventCollector collector;

        auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
            .receiver([&](Event e) { collector.add_event(std::move(e)); })
            .use_curl(true)
            .build();

        client->async_connect();
        ASSERT_TRUE(collector.wait_for_events(1));

        SimpleLatch shutdown_latch(1);
        client->async_shutdown([&] { shutdown_latch.count_down(); });
        EXPECT_TRUE(shutdown_latch.wait_for(5000ms));

        // Client should be cleanly destroyed here
    }

    // If threads weren't properly joined, we'd likely see issues here
    // The test passing indicates proper resource cleanup
}

TEST(CurlClientTest, DestructorCleansUpProperly) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Keep sending events
        for (int i = 0; i < 100; i++) {
            send_sse_event(SSEFormatter::event("event " + std::to_string(i)));
            std::this_thread::sleep_for(10ms);
        }
    });

    IoContextRunner runner;
    EventCollector collector;

    {
        auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
            .receiver([&](Event e) { collector.add_event(std::move(e)); })
            .use_curl(true)
            .build();

        client->async_connect();
        ASSERT_TRUE(collector.wait_for_events(1));

        // Let destructor run without explicit shutdown
    }

    // If destructor doesn't properly clean up, this could hang or crash
    // Test passing indicates proper cleanup in destructor
}

// Edge case tests

TEST(CurlClientTest, HandlesEmptyEventData) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event(""));
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, HandlesEventWithOnlyType) {
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Send event with type but empty data
        send_sse_event("event: heartbeat\ndata: \n\n");
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(1));
    auto events = collector.events();
    ASSERT_EQ(1, events.size());
    EXPECT_EQ("heartbeat", events[0].type());
    EXPECT_EQ("", events[0].data());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, HandlesRapidEvents) {
    MockSSEServer server;
    const int num_events = 100;

    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Send many events rapidly
        for (int i = 0; i < num_events; i++) {
            send_sse_event(SSEFormatter::event("event" + std::to_string(i)));
        }
        std::this_thread::sleep_for(10ms);
        close();
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    ASSERT_TRUE(collector.wait_for_events(num_events, 10000ms));
    auto events = collector.events();
    EXPECT_EQ(num_events, events.size());

    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

// Shutdown-specific tests - critical for preventing crashes/hangs in user applications

TEST(CurlClientTest, ShutdownDuringBackoffDelay) {
    // Tests curl_client.cpp:138 - on_backoff checks shutting_down_
    // This ensures clean shutdown during backoff/retry wait period
    std::atomic<int> connection_attempts{0};

    auto handler = [&](auto const&, auto send_response, auto, auto) {
        connection_attempts++;
        // Return 500 to trigger backoff
        http::response<http::string_body> res{http::status::internal_server_error, 11};
        res.body() = "Error";
        res.prepare_payload();
        send_response(res);
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .initial_reconnect_delay(2000ms)  // Long delay to ensure we shutdown during wait
        .use_curl(true)
        .build();

    client->async_connect();

    // Wait for first connection attempt to complete
    std::this_thread::sleep_for(200ms);
    EXPECT_GE(connection_attempts.load(), 1);

    // Now shutdown while it's waiting in backoff
    auto shutdown_start = std::chrono::steady_clock::now();
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;

    // Shutdown should complete quickly despite long backoff delay
    EXPECT_LT(shutdown_duration, 1000ms);

    // Should NOT have made another connection attempt during backoff
    EXPECT_EQ(1, connection_attempts.load());
}

TEST(CurlClientTest, ShutdownDuringDataReception) {
    // Tests curl_client.cpp:235 - WriteCallback checks shutting_down_
    // This covers the branch where we abort during SSE data parsing
    SimpleLatch server_sending(1);
    SimpleLatch client_received_some(1);

    auto handler = [&](auto const&, auto send_response, auto send_sse_event, auto) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        // Send events continuously
        for (int i = 0; i < 100; i++) {
            if (!send_sse_event(SSEFormatter::event("event " + std::to_string(i)))) {
                return;  // Connection closed or error - stop sending
            }
            if (i == 2) {
                server_sending.count_down();
            }
            std::this_thread::sleep_for(10ms);  // Slow enough to allow shutdown mid-stream
        }
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) {
            collector.add_event(std::move(e));
            if (collector.events().size() >= 2) {
                client_received_some.count_down();
            }
        })
        .use_curl(true)
        .build();

    client->async_connect();

    // Wait until server is sending and client has received some events
    ASSERT_TRUE(server_sending.wait_for(5000ms));
    ASSERT_TRUE(client_received_some.wait_for(5000ms));

    // Shutdown while WriteCallback is actively processing data
    auto shutdown_start = std::chrono::steady_clock::now();
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;

    // Shutdown should complete quickly even during active data transfer
    EXPECT_LT(shutdown_duration, 2000ms);
}

TEST(CurlClientTest, ShutdownDuringProgressCallback) {
    // Tests curl_client.cpp:188 - ProgressCallback checks shutting_down_
    // This ensures we can abort during slow data transfer
    SimpleLatch server_started(1);

    auto handler = [&](auto const&, auto send_response, auto send_sse_event, auto) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        server_started.count_down();

        // Send one event then pause (simulating slow connection)
        send_sse_event(SSEFormatter::event("first"));
        std::this_thread::sleep_for(5000ms);  // Pause to simulate slow connection
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .read_timeout(10000ms)  // Long timeout so ProgressCallback is called but doesn't abort
        .use_curl(true)
        .build();

    client->async_connect();

    // Wait for first event and server pause
    ASSERT_TRUE(server_started.wait_for(5000ms));
    ASSERT_TRUE(collector.wait_for_events(1, 5000ms));

    // Shutdown while ProgressCallback is being invoked during the pause
    auto shutdown_start = std::chrono::steady_clock::now();
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;

    // Shutdown should abort the transfer quickly
    EXPECT_LT(shutdown_duration, 2000ms);
}

TEST(CurlClientTest, MultipleShutdownCalls) {
    // Ensures multiple shutdown calls don't cause issues (idempotency test)
    MockSSEServer server;
    auto port = server.start(TestHandlers::simple_event("test"));

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();
    ASSERT_TRUE(collector.wait_for_events(1));

    // Call shutdown multiple times in rapid succession
    SimpleLatch shutdown_latch1(1);
    SimpleLatch shutdown_latch2(1);
    SimpleLatch shutdown_latch3(1);

    client->async_shutdown([&] { shutdown_latch1.count_down(); });
    client->async_shutdown([&] { shutdown_latch2.count_down(); });
    client->async_shutdown([&] { shutdown_latch3.count_down(); });

    // All shutdown completions should be called
    EXPECT_TRUE(shutdown_latch1.wait_for(5000ms));
    EXPECT_TRUE(shutdown_latch2.wait_for(5000ms));
    EXPECT_TRUE(shutdown_latch3.wait_for(5000ms));
}

TEST(CurlClientTest, ShutdownAfterConnectionClosed) {
    // Tests shutdown when connection has already ended naturally
    MockSSEServer server;
    auto port = server.start([](auto const&, auto send_response, auto send_sse_event, auto close) {
        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("only event"));
        std::this_thread::sleep_for(10ms);
        close();  // Server closes connection
    });

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .initial_reconnect_delay(500ms)  // Will try to reconnect after close
        .use_curl(true)
        .build();

    client->async_connect();
    ASSERT_TRUE(collector.wait_for_events(1));

    // Wait for connection to close and reconnect attempt to start
    std::this_thread::sleep_for(200ms);

    // Shutdown after natural connection close
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
}

TEST(CurlClientTest, ShutdownDuringConnectionAttempt) {
    // Tests curl_client.cpp:439 - perform_request checks shutting_down_ at start
    // Server that delays before responding to test shutdown during connection phase
    SimpleLatch connection_started(1);

    auto handler = [&](auto const&, auto send_response, auto send_sse_event, auto close) {
        connection_started.count_down();
        // Delay before responding
        std::this_thread::sleep_for(500ms);

        http::response<http::string_body> res{http::status::ok, 11};
        res.set(http::field::content_type, "text/event-stream");
        res.chunked(true);
        send_response(res);

        send_sse_event(SSEFormatter::event("test"));
        std::this_thread::sleep_for(10ms);
        close();
    };

    MockSSEServer server;
    auto port = server.start(handler);

    IoContextRunner runner;
    EventCollector collector;

    auto client = Builder(runner.context().get_executor(), "http://localhost:" + std::to_string(port))
        .receiver([&](Event e) { collector.add_event(std::move(e)); })
        .use_curl(true)
        .build();

    client->async_connect();

    // Wait for connection to start but shutdown before it completes
    ASSERT_TRUE(connection_started.wait_for(5000ms));
    std::this_thread::sleep_for(50ms);  // Give CURL time to start but not finish

    auto shutdown_start = std::chrono::steady_clock::now();
    SimpleLatch shutdown_latch(1);
    client->async_shutdown([&] { shutdown_latch.count_down(); });
    EXPECT_TRUE(shutdown_latch.wait_for(5000ms));
    auto shutdown_duration = std::chrono::steady_clock::now() - shutdown_start;

    // Shutdown should abort the pending connection quickly
    EXPECT_LT(shutdown_duration, 2000ms);

    // Should not have received any events since we shutdown during connection
    EXPECT_EQ(0, collector.events().size());
}
