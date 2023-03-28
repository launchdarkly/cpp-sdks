#include "server.hpp"

#include "console_backend.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast.hpp>

#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;

using launchdarkly::ConsoleBackend;

using launchdarkly::LogLevel;

int main(int argc, char* argv[]) {
    launchdarkly::Logger logger{
        std::make_unique<ConsoleBackend>("sse-contract-tests")};

    try {
        net::io_context ioc{1};

        auto s = std::make_shared<server>(ioc, "0.0.0.0", "8111", logger);
        s->add_capability("headers");
        s->add_capability("comments");
        s->add_capability("report");
        s->add_capability("post");
        s->run();

        net::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&](beast::error_code const&, int) {
            LD_LOG(logger, LogLevel::kInfo) << "shutting down..";
            ioc.stop();
        });

        ioc.run();
        LD_LOG(logger, LogLevel::kInfo) << "bye!";

    } catch (std::exception const& e) {
        LD_LOG(logger, LogLevel::kError) << e.what();
        return EXIT_FAILURE;
    }
}
