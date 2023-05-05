#include "session.hpp"
#include <boost/algorithm/string/erase.hpp>
#include <boost/asio/dispatch.hpp>
#include <iostream>

const std::string kEntityPath = "/entity/";

namespace net = boost::asio;

using launchdarkly::LogLevel;

Session::Session(foxy::server_session& session,
                 EntityManager& manager,
                 std::vector<std::string> caps,
                 launchdarkly::Logger& logger)
    : session_(session),
      frame_(std::make_unique<Frame>()),
      manager_(manager),
      caps_(std::move(caps)),
      logger_(logger) {
    // LD_LOG(logger_, LogLevel::kDebug) << "session: created";
}

// Session::~Session() {
//     // LD_LOG(logger_, LogLevel::kDebug) << "session: destroyed";
// }

void Session::start() {
    // LD_LOG(logger_, LogLevel::kDebug) << "session: start";
}

void Session::stop() {
    // LD_LOG(logger_, LogLevel::kDebug) << "session: stop";
    //    session_.async_shutdown(
    //        beast::bind_front_handler(&Session::on_stop, shared_from_this()));
}

http::response<http::string_body> Session::generate_response(
    http::request<http::string_body>& req) {
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
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.keep_alive(req.keep_alive());
        res.set("Location", kEntityPath + id);
        res.prepare_payload();
        return res;
    };

    auto const destroy_entity_response = [&req](bool erased) {
        auto status = erased ? http::status::ok : http::status::not_found;
        http::response<http::string_body> res{status, req.version()};
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    };

    auto const shutdown_server_response = [&req]() {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.keep_alive(false);
        res.prepare_payload();
        return res;
    };

    if (req.method() == http::verb::get && req.target() == "/") {
        return capabilities_response(caps_);
    }

    if (req.method() == http::verb::head && req.target() == "/") {
        return http::response<http::string_body>{http::status::ok,
                                                 req.version()};
    }

    if (req.method() == http::verb::delete_ && req.target() == "/") {
        shutdown_requested_ = true;
        return shutdown_server_response();
    }

    if (req.method() == http::verb::post && req.target() == "/") {
        try {
            auto json = nlohmann::json::parse(req.body());
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
