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

    std::string port = "8123";
    if (argc == 2) {
        port =
            argv[1];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    try {
        net::io_context ioc{1};

        auto srv = std::make_shared<server>(ioc, "0.0.0.0", port, logger);
        srv->add_capability("headers");
        srv->add_capability("comments");
        srv->add_capability("report");
        srv->add_capability("post");
        srv->add_capability("read-timeout");
        srv->run();

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
