#include "session.hpp"
#include <boost/algorithm/string/erase.hpp>
#include <boost/asio/dispatch.hpp>
#include <iostream>

const std::string kEntityPath = "/entity/";

namespace net = boost::asio;

using launchdarkly::LogLevel;

Session::Session(tcp::socket&& socket,
                 EntityManager& manager,
                 std::vector<std::string> caps,
                 launchdarkly::Logger& logger)
    : stream_{std::move(socket)},
      manager_{manager},
      capabilities_{std::move(caps)},
      on_shutdown_cb_{},
      shutdown_requested_{false},
      logger_{logger} {
    LD_LOG(logger_, LogLevel::kDebug) << "session: created";
}

Session::~Session() {
    LD_LOG(logger_, LogLevel::kDebug) << "session: destroyed";
}

void Session::start() {
    LD_LOG(logger_, LogLevel::kDebug) << "session: start";
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&Session::do_read, shared_from_this()));
}

void Session::stop() {
    LD_LOG(logger_, LogLevel::kDebug) << "session: stop";
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(
                      &Session::do_stop, shared_from_this(), "stop requested"));
}

void Session::do_stop(char const* reason) {
    LD_LOG(logger_, LogLevel::kDebug)
        << "session: closing socket (" << reason << ")";
    stream_.close();
}

void Session::do_read() {
    request_ = {};

    LD_LOG(logger_, LogLevel::kDebug) << "session: awaiting request";
    http::async_read(
        stream_, buffer_, request_,
        beast::bind_front_handler(&Session::on_read, shared_from_this()));
}

void Session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        return do_stop("end of stream");
    }

    if (ec) {
        return do_stop("read failed");
    }

    send_response(handle_request(std::move(request_)));
}

void Session::send_response(http::message_generator&& msg) {
    beast::async_write(
        stream_, std::move(msg),
        beast::bind_front_handler(&Session::on_write, shared_from_this(),
                                  request_.keep_alive()));
}

void Session::on_write(bool keep_alive,
                       beast::error_code ec,
                       std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (shutdown_requested_ && on_shutdown_cb_) {
        LD_LOG(logger_, LogLevel::kDebug)
            << "session: client requested server termination";
        on_shutdown_cb_();
    }

    if (ec) {
        return do_stop("write failed");
    }

    if (!keep_alive) {
        return do_stop("client dropped connection");
    }

    do_read();
}

http::message_generator Session::handle_request(
    http::request<http::string_body>&& req) {
    auto const bad_request = [&req](beast::string_view why) {
        http::response<http::string_body> res{http::status::bad_request,
                                              req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = nlohmann::json{"error", why}.dump();
        res.prepare_payload();
        return res;
    };

    auto const not_found = [&req](beast::string_view target) {
        http::response<http::string_body> res{http::status::not_found,
                                              req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() =
            "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    auto const server_error = [&req](beast::string_view what) {
        http::response<http::string_body> res{
            http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    auto const capabilities_response = [&req](std::vector<std::string> const&
                                                  caps) {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = nlohmann::json{{"capabilities", caps}}.dump();
        res.prepare_payload();
        return res;
    };

    auto const create_entity_response = [&req](std::string const& id) {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.keep_alive(req.keep_alive());
        res.set("Location", kEntityPath + id);
        res.prepare_payload();
        return res;
    };

    auto const destroy_entity_response = [&req](bool erased) {
        auto status = erased ? http::status::ok : http::status::not_found;
        http::response<http::empty_body> res{status, req.version()};
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    };

    auto const shutdown_server_response = [&req]() {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.keep_alive(false);
        res.prepare_payload();
        return res;
    };

    if (req.method() == http::verb::get && req.target() == "/") {
        return capabilities_response(capabilities_);
    }

    if (req.method() == http::verb::head && req.target() == "/") {
        return http::response<http::empty_body>{http::status::ok,
                                                req.version()};
    }

    if (req.method() == http::verb::delete_ && req.target() == "/") {
        shutdown_requested_ = true;
        return shutdown_server_response();
    }

    if (req.method() == http::verb::post && req.target() == "/") {
        try {
            auto json = nlohmann::json::parse(request_.body());
            auto params = json.get<ConfigParams>();
            if (auto id = manager_.create(std::move(params))) {
                return create_entity_response(*id);
            } else {
                return server_error("couldn't create client entity");
            }
        } catch (nlohmann::json::exception& e) {
            return bad_request("unable to parse config JSON");
        }
    }

    if (req.method() == http::verb::delete_ &&
        req.target().starts_with(kEntityPath)) {
        std::string id = req.target();
        boost::erase_first(id, kEntityPath);
        bool erased = manager_.destroy(id);
        return destroy_entity_response(erased);
    }

    return not_found(req.target());
}
