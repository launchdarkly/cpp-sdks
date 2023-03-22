#include "server.hpp"
#include "session.hpp"

#include <boost/bind.hpp>
#include <iostream>


server::server(net::any_io_executor executor,
               boost::asio::ip::address const& address,
               unsigned short port)
    : acceptor_{executor, {address, port}},
      stopped_{false},
      signals_{executor},
      manager_{executor},
      caps_{} {
    signals_.add(SIGTERM);
    signals_.add(SIGINT);
    signals_.async_wait(boost::bind(&server::stop, this));
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
}

void server::add_capability(std::string cap) {
    caps_.push_back(std::move(cap));
}

void server::start() {
    acceptor_.listen();
    accept_connection();
}

void server::stop() {
    boost::asio::post(acceptor_.get_executor(), [this]() {
        std::cout << "stopping server\n";
        acceptor_.cancel();
        stopped_ = true;
        std::cout << "server stopped\n";
    });
}

void server::accept_connection() {
    acceptor_.async_accept([this](beast::error_code ec, tcp::socket peer) {
        if (!ec && !stopped_) {
            std::make_shared<Session>(std::move(peer), manager_, caps_)
                ->start();
            accept_connection();
        }
    });
}
