#pragma once

#include <launchdarkly/sse/event.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>

#include <functional>
#include <memory>
#include <string>

namespace launchdarkly::sse {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class Client;

/**
 * Builder can be used to create an instance of Client. Minimal example:
 * @code
 * auto client = launchdarkly::sse::Builder(executor,
 * "https://example.com").build();
 * @endcode
 */
class Builder {
   public:
    using EventReceiver = std::function<void(launchdarkly::sse::Event)>;
    using LogCallback = std::function<void(std::string)>;

    /**
     * Create a builder for the given URL. If the port is omitted, 443 is
     * assumed for https scheme while 80 is assumed for http scheme.
     *
     * Example: https://example.com:8123/endpoint
     *
     * @param ioc Executor for the Client.
     * @param url Server-Sent-EventConfig server URL.
     */
    Builder(net::any_io_executor ioc, std::string url);

    /**
     * Add a custom header to the initial request. The following headers
     * are added by default and can be overridden:
     *
     * User-Agent: the default Boost.Beast user agent.
     * Accept: text/event-stream
     * Cache-Control: no-cache
     *
     * Note that Content-Type and
     *
     * @param name Header name.
     * @param value Header value.
     * @return Reference to this builder.
     */
    Builder& header(std::string const& name, std::string const& value);

    /**
     * Specifies a request body. The body is sent when the method is POST or
     * REPORT.
     * @return Reference to this builder.
     */
    Builder& body(std::string);

    /**
     * Specifies the maximum time duration between subsequent reads from the
     * stream. A read counts as receiving any amount of bytes.
     * @param timeout
     * @return Reference to this builder.
     */
    Builder& read_timeout(std::chrono::milliseconds timeout);

    /**
     * Specify the method for the initial request. The default method is GET.
     * @param verb The HTTP method.
     * @return Reference to this builder.
     */
    Builder& method(http::verb verb);

    /**
     * Specify a receiver of events generated by the Client. For example:
     * @code
     * builder.receiver([](launchdarkly::sse::Event event) -> void {
     *        std::cout << event.type() << ": " << event.data() << std::endl;
     * });
     * @endcode
     *
     * @return Reference to this builder.
     */
    Builder& receiver(EventReceiver);

    /**
     * Specify a logging callback for the Client.
     * @param callback Callback to receive a string from the Client.
     * @return Reference to this builder.
     */
    Builder& logger(LogCallback callback);

    /**
     * Builds a Client. The shared pointer is necessary to extend the lifetime
     * of the Client to encompass each asynchronous operation that it performs.
     * @return New client; call run() to kickoff the connection process and
     * begin reading.
     */
    std::shared_ptr<Client> build();

   private:
    std::string url_;
    net::any_io_executor executor_;
    http::request<http::string_body> request_;
    std::optional<std::chrono::milliseconds> read_timeout_;
    LogCallback logging_cb_;
    EventReceiver receiver_;
};

/**
 * Client is a long-lived Server-Sent-EventConfig (EventSource) client which
 * reads from an event stream and dispatches events to a user-specified
 * receiver.
 */
class Client {
   public:
    virtual ~Client() = default;
    /**
     * Kicks off a connection to the server and begins reading the event stream.
     * The provided event receiver and logging callbacks will be invoked from
     * the thread that is servicing the Client's executor.
     */
    virtual void run() = 0;
    /**
     * Closes the stream.
     */
    virtual void close() = 0;
};

}  // namespace launchdarkly::sse
