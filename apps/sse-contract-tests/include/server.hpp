#pragma once

#include "entity_manager.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <vector>


using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class server {
    tcp::acceptor acceptor_;
    bool stopped_;
    boost::asio::signal_set signals_;
    EntityManager manager_;
    std::vector<std::string> caps_;

   public:
    server(boost::asio::any_io_executor executor,
           boost::asio::ip::address const& address,
           unsigned short port);

    void add_capability(std::string cap);
    void start();
    void stop();

   private:
    void accept_connection();
};
