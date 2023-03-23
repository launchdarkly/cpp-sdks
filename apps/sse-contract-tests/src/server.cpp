#include "server.hpp"
#include "session.hpp"

#include <boost/asio/dispatch.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <boost/bind.hpp>
#include <iostream>

using launchdarkly::LogLevel;

server::server(net::io_context& ioc,
               std::string const& address,
               std::string const& port,
               launchdarkly::Logger& logger)
    : ioc_{ioc},
      acceptor_{ioc},
      entity_manager_{ioc.get_executor(), logger},
      caps_{},
      logger_{logger} {
    beast::error_code ec;

    tcp::resolver resolver{ioc_};
    tcp::endpoint endpoint = *resolver.resolve(address, port, ec).begin();
    if (ec) {
        fail(ec, "resolve");
        return;
    }
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }
    acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }

    LD_LOG(logger_, LogLevel::kInfo)
        << "server: listening on " << address << ":" << port;
}

void server::fail(beast::error_code ec, char const* what) {
    LD_LOG(logger_, LogLevel::kError) << "server: " << what << ": " << ec.message();
}

void server::add_capability(std::string cap) {
    LD_LOG(logger_, LogLevel::kDebug) << "server: test capability: <" << cap << ">";
    caps_.push_back(std::move(cap));
}

void server::run() {
    LD_LOG(logger_, LogLevel::kDebug) << "server: run requested";
    net::dispatch(
        acceptor_.get_executor(),
        beast::bind_front_handler(&server::do_accept, shared_from_this()));
}

void server::do_accept() {
    LD_LOG(logger_, LogLevel::kDebug) << "server: waiting for connection";
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&server::on_accept, shared_from_this()));
}

void server::on_accept(boost::system::error_code const& ec,
                       tcp::socket socket) {
    if (!acceptor_.is_open()) {
        return;
    }
    if (ec) {
        fail(ec, "accept");
        return;
    }


    auto session =
        std::make_shared<Session>(std::move(socket), entity_manager_, caps_, logger_);

    session->on_shutdown([this]() {
        LD_LOG(logger_, LogLevel::kDebug) << "server: terminating";
        ioc_.stop();
    });

    session->start();

    do_accept();
}
