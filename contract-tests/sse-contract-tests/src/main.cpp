#include "server.hpp"

#include <launchdarkly/logging/console_backend.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/lexical_cast.hpp>

#include <memory>

namespace net = boost::asio;
namespace beast = boost::beast;

using launchdarkly::logging::ConsoleBackend;

using launchdarkly::LogLevel;

int main(int argc, char* argv[]) {
    launchdarkly::Logger logger{
        std::make_unique<ConsoleBackend>("sse-contract-tests")};

    std::string const default_port = "8123";
    std::string port = default_port;
    bool use_curl = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (arg == "--use-curl") {
            use_curl = true;
            LD_LOG(logger, LogLevel::kInfo) << "Using CURL implementation for SSE clients";
        } else if (i == 1 && arg.find("--") != 0) {
            // First non-flag argument is the port
            port = arg;
        }
    }

    try {
        net::io_context ioc{1};

        server srv(ioc, "0.0.0.0", boost::lexical_cast<unsigned short>(port),
                   logger, use_curl);

        srv.add_capability("headers");
        srv.add_capability("comments");
        srv.add_capability("report");
        srv.add_capability("post");
        srv.add_capability("reconnection");
        srv.add_capability("read-timeout");

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
