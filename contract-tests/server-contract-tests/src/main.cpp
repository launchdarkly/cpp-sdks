#include "server.hpp"

#include <launchdarkly/logging/console_backend.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <boost/lexical_cast.hpp>

#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;

using launchdarkly::logging::ConsoleBackend;

using launchdarkly::LogLevel;

int main(int argc, char* argv[]) {
    launchdarkly::Logger logger{
        std::make_unique<ConsoleBackend>("server-contract-tests")};

    const std::string default_port = "8123";
    std::string port = default_port;
    if (argc == 2) {
        port =
            argv[1];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }

    try {
        net::io_context ioc{1};

        auto p = boost::lexical_cast<unsigned short>(port);
        server srv(ioc, "0.0.0.0", p, logger);

        srv.add_capability("server-side");
        srv.add_capability("strongly-typed");
        srv.add_capability("context-type");
        srv.add_capability("service-endpoints");
        srv.add_capability("tags");
        srv.add_capability("server-side-polling");
        srv.add_capability("inline-context");

        net::signal_set signals{ioc, SIGINT, SIGTERM};

        boost::asio::spawn(ioc.get_executor(), [&](auto yield) mutable {
            signals.async_wait(yield);
            LD_LOG(logger, LogLevel::kInfo) << "shutting down..";
            srv.shutdown();
        });

        ioc.run();
        LD_LOG(logger, LogLevel::kInfo) << "bye!";

    } catch (boost::bad_lexical_cast&) {
        LD_LOG(logger, LogLevel::kError)
            << "invalid port (" << port
            << "), provide a number (no arguments defaults "
               "to port "
            << default_port << ")";
        return EXIT_FAILURE;
    } catch (std::exception const& e) {
        LD_LOG(logger, LogLevel::kError) << e.what();
        return EXIT_FAILURE;
    }
}
