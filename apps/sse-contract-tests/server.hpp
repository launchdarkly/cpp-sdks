#pragma once

#include "stream_entity.hpp"
#include "session.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class server
{
public:
    server(net::any_io_executor executor, boost::asio::ip::address const& address, unsigned short port):
        acceptor_(executor, {address, port}),
        stopped_(false),
        signals_{executor},
        manager_{executor}
    {
        signals_.add(SIGTERM);
        signals_.add(SIGINT);
        signals_.async_wait(boost::bind(&server::stop, this));
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
    }

    void add_capability(std::string cap) {
        caps_.push_back(cap);
    }

    void start_accepting()
    {
        acceptor_.listen();
        accept_loop();
    }

    void stop()
    {
        boost::asio::post(acceptor_.get_executor(), [this]()
        {
            std::cout << "Stopping server\n";
            acceptor_.cancel();
            stopped_ = true;
            std::cout << "Server stopped\n";
        });
    }

private:
    void accept_loop()
    {
        acceptor_.async_accept([this](beast::error_code ec, tcp::socket peer){
           if (!ec) {
               if (!stopped_) {
                   std::make_shared<session>(std::move(peer), manager_, caps_)->start();
                   accept_loop();
               }
           }
       });
    }

    tcp::acceptor acceptor_;
    boost::asio::signal_set signals_;
    bool stopped_;
    entity_manager manager_;
    std::vector<std::string> caps_;
};
