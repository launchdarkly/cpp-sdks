#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast.hpp>
#include "entity_manager.hpp"
#include "logger.hpp"

namespace net = boost::asio;  // from <boost/asio.hpp>

#include <vector>

using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class server : public std::enable_shared_from_this<server> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    EntityManager entity_manager_;
    std::vector<std::string> caps_;
    launchdarkly::Logger& logger_;

   public:
    /**
     * Constructs a server, which stands up a REST API at the given
     * port and address.
     * @param ioc IO context.
     * @param address Address to bind.
     * @param port Port to bind.
     * @param logger Logger.
     */
    server(net::io_context& ioc,
           std::string const& address,
           std::string const& port,
           launchdarkly::Logger& logger);
    /**
     * Advertise an optional test-harness capability, such as "comments".
     * @param cap
     */
    void add_capability(std::string cap);
    /**
     * Begins an async operation to start accepting requests.
     */
    void run();
   private:
    void do_accept();
    void on_accept(boost::system::error_code const& ec, tcp::socket socket);
    void fail(boost::beast::error_code ec, char const* what);
};
