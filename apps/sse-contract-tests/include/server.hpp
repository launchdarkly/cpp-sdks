#pragma once

#include "entity_manager.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/io_context.hpp>

namespace net = boost::asio;       // from <boost/asio.hpp>

#include <vector>

using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class server : public std::enable_shared_from_this<server> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    bool stopped_;
    EntityManager entity_manager_;
    std::vector<std::string> caps_;

   public:
    server(net::io_context& ioc, std::string const& address, std::string const& port);
    void add_capability(std::string cap);
    void run();
    void stop();

   private:
    void do_accept();
    void on_accept(const boost::system::error_code& ec, tcp::socket socket);

    void do_stop();
};
