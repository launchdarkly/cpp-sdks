#include "server.hpp"
#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include "session.hpp"

using launchdarkly::LogLevel;

server::server(net::io_context& ioc,
               std::string const& address,
               unsigned short port,
               launchdarkly::Logger& logger)
    : manager_(ioc.get_executor(), logger),
      listener_{ioc.get_executor(),
                tcp::endpoint(boost::asio::ip::make_address(address), port)},
      logger_{logger} {
    LD_LOG(logger_, LogLevel::kInfo)
        << "server: listening on " << address << ":" << port;
    listener_.async_accept([this](auto& server) {
        return Session(server, manager_, caps_, logger_);
    });
}

void server::add_capability(std::string cap) {
    LD_LOG(logger_, LogLevel::kDebug)
        << "server: test capability: <" << cap << ">";
    caps_.push_back(std::move(cap));
}

void server::shutdown() {
    listener_.shutdown();
}
