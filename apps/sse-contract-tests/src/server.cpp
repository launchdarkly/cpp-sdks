#include "server.hpp"
#include "session.hpp"

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/strand.hpp>
#include <boost/bind.hpp>
#include <iostream>

using launchdarkly::LogLevel;

server::server(net::io_context& ioc,
               std::string const& address,
               unsigned short port,
               launchdarkly::Logger& logger)
    : ioc_{ioc},
      listener_{ioc.get_executor(),
                tcp::endpoint(boost::asio::ip::make_address(address), port)},
      entity_manager_{ioc.get_executor(), logger},
      caps_{},
      logger_{logger} {}

void server::fail(beast::error_code ec, char const* what) {
    LD_LOG(logger_, LogLevel::kError)
        << "server: " << what << ": " << ec.message();
}

void server::add_capability(std::string cap) {
    LD_LOG(logger_, LogLevel::kDebug)
        << "server: test capability: <" << cap << ">";
    caps_.push_back(std::move(cap));
}

void server::run() {
    //    LD_LOG(logger_, LogLevel::kInfo)
    //        << "server: listening on " << address << ":" << port

    listener_.async_accept([this](auto& server) {
        return Session(server, entity_manager_, caps_, logger_);
    });
}

void server::shutdown() {
    listener_.shutdown();
}

void server::on_accept(foxy::server_session& server_session) {
    //    auto session = std::make_shared<Session>(server_session,
    //    entity_manager_,
    //                                             caps_, logger_);
    //
    //    session->on_shutdown([this]() {
    //        LD_LOG(logger_, LogLevel::kDebug) << "server: terminating";
    //        ioc_.stop();
    //    });
    //
    //    session->start();
}
