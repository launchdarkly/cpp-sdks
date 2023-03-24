#pragma once

#include <launchdarkly/sse/event.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>

#include <functional>
#include <memory>
#include <string>

namespace launchdarkly::sse {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class Client;

class Builder {
   public:
    using EventReceiver = std::function<void(launchdarkly::sse::Event)>;
    using LogCallback = std::function<void(std::string)>;

    Builder(net::any_io_executor ioc, std::string url);
    Builder& header(std::string const& name, std::string const& value);
    Builder& method(http::verb verb);
    Builder& receiver(EventReceiver);
    Builder& logging(LogCallback callback);
    std::shared_ptr<Client> build();

   private:
    std::string url_;
    net::any_io_executor executor_;
    http::request<http::empty_body> request_;
    LogCallback logging_cb_;
    EventReceiver receiver_;
};

class Client {
   public:
    virtual ~Client() = default;
    virtual void run() = 0;
    virtual void close() = 0;
};

}  // namespace launchdarkly::sse
