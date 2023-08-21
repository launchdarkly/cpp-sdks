#pragma once

#include "entity_manager.hpp"

#include <launchdarkly/logging/logger.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast.hpp>

#include <foxy/listener.hpp>

#include <vector>

namespace net = boost::asio;  // from <boost/asio.hpp>

using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class server {
    EntityManager manager_;
    launchdarkly::foxy::listener listener_;
    std::vector<std::string> caps_;
    launchdarkly::Logger& logger_;

   public:
    /**
     * Constructs a server, which stands up a REST API at the given
     * port and address. The server is ready to accept connections upon
     * construction.
     * @param ioc IO context.
     * @param address Address to bind.
     * @param port Port to bind.
     * @param logger Logger.
     */
    server(net::io_context& ioc,
           std::string const& address,
           unsigned short port,
           launchdarkly::Logger& logger);
    /**
     * Advertise an optional test-harness capability, such as "comments".
     * @param cap
     */
    void add_capability(std::string cap);

    /**
     * Shuts down the server.
     */
    void shutdown();
};
