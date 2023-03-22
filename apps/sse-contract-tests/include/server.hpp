#pragma once


#include "entity_manager.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>

#include <vector>


namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

class server {
    tcp::acceptor acceptor_;
    bool stopped_;
    boost::asio::signal_set signals_;
    EntityManager manager_;
    std::vector<std::string> caps_;

   public:
    server(net::any_io_executor executor,
           boost::asio::ip::address const& address,
           unsigned short port);

    void add_capability(std::string cap);
    void start();
    void stop();

   private:
    void accept_connection();
};
