#include "server.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast.hpp>
#include <iostream>
#include <thread>

namespace net = boost::asio;
namespace beast = boost::beast;

int main(int argc, char* argv[]) {
    try {
        net::io_context ioc{1};

        auto s = std::make_shared<server>(ioc, "0.0.0.0", "8111");
        s->add_capability("headers");
        s->run();

        net::signal_set signals{ioc, SIGINT, SIGTERM};
        signals.async_wait([&](beast::error_code const&, int) { ioc.stop(); });

        ioc.run();
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
