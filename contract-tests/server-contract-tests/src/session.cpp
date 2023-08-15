#include "session.hpp"

#include <launchdarkly/server_side/client.hpp>

#include <boost/algorithm/string/erase.hpp>
#include <boost/asio/dispatch.hpp>

#include <iostream>

const std::string kEntityPath = "/entity/";

namespace net = boost::asio;

Session::Session(launchdarkly::foxy::server_session& session,
                 EntityManager& manager,
                 std::vector<std::string>& caps,
                 launchdarkly::Logger& logger)
    : session_(session),
      frame_(std::make_unique<Frame>()),
      manager_(manager),
      caps_(caps),
      logger_(logger) {}

std::optional<Session::Response> Session::generate_response(Request& req) {
    auto const bad_request = [&req](beast::string_view why) {
        Response res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.keep_alive(req.keep_alive());
        res.body() = nlohmann::json{"error", why}.dump();
        res.prepare_payload();
        return res;
    };

    auto const not_found = [&req](beast::string_view target) {
        Response res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() =
            "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    auto const server_error = [&req](beast::string_view what) {
        Response res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    auto const capabilities_response =
        [&req](std::vector<std::string> const& caps, std::string const& name,
               std::string const& version) {
            Response res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = nlohmann::json{
                {"capabilities", caps},
                {"name", name},
                {"clientVersion",
                 version}}.dump();
            res.prepare_payload();
            return res;
        };

    auto const create_entity_response = [&req](std::string const& id) {
        Response res{http::status::ok, req.version()};
        res.keep_alive(req.keep_alive());
        res.set("Location", kEntityPath + id);
        res.prepare_payload();
        return res;
    };

    auto const destroy_entity_response = [&req](bool erased) {
        auto status = erased ? http::status::ok : http::status::not_found;
        Response res{status, req.version()};
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        return res;
    };

    if (req.method() == http::verb::get && req.target() == "/") {
        return capabilities_response(caps_, "cpp-server-sdk", launchdarkly::server_side::Client::Version());
    }

    if (req.method() == http::verb::head && req.target() == "/") {
        return http::response<http::string_body>{http::status::ok,
                                                 req.version()};
    }

    if (req.method() == http::verb::delete_ && req.target() == "/") {
        return std::nullopt;
    }

    if (req.method() == http::verb::post && req.target() == "/") {
        try {
            auto json = nlohmann::json::parse(req.body());
            auto params = json.get<CreateInstanceParams>();
            if (auto entity_id = manager_.create(params.configuration)) {
                return create_entity_response(*entity_id);
            }
            return server_error("couldn't create client entity");
        } catch (nlohmann::json::exception& e) {
            return bad_request("unable to parse config JSON");
        }
    }

    if (req.method() == http::verb::post &&
        req.target().starts_with(kEntityPath)) {
        std::string entity_id = req.target();
        boost::erase_first(entity_id, kEntityPath);

        try {
            auto json = nlohmann::json::parse(req.body());
            auto params = json.get<CommandParams>();
            tl::expected<nlohmann::json, std::string> res =
                manager_.command(entity_id, params);
            if (res.has_value()) {
                auto response = http::response<http::string_body>{
                    http::status::ok, req.version()};
                response.body() = res->dump();
                response.prepare_payload();
                return response;
            } else {
                return bad_request(res.error());
            }
        } catch (nlohmann::json::exception& e) {
            return bad_request("unable to parse config JSON");
        }
    }

    if (req.method() == http::verb::delete_ &&
        req.target().starts_with(kEntityPath)) {
        std::string entity_id = req.target();
        boost::erase_first(entity_id, kEntityPath);
        bool erased = manager_.destroy(entity_id);
        return destroy_entity_response(erased);
    }

    return not_found(req.target());
}
