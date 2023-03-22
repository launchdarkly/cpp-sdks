#include "session.hpp"
#include <boost/algorithm/string/erase.hpp>
#include <iostream>

const std::string ENTITY_PATH = "/entity/";

http::message_generator session::handle_request(
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
        res.set("Location", ENTITY_PATH + id);
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

    if (req.method() == http::verb::get && req.target() == "/") {
        return capabilities_response(capabilities_);
    }

    if (req.method() == http::verb::head && req.target() == "/") {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        return res;
    }

    if (req.method() == http::verb::delete_ && req.target() == "/") {
        // not clean, but doesn't matter from the test-harness's perspective.
        std::raise(SIGTERM);
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
        req.target().starts_with(ENTITY_PATH)) {
        std::string id = req.target();
        boost::erase_first(id, ENTITY_PATH);
        bool erased = manager_.destroy(id);
        return destroy_entity_response(erased);
    }

    return bad_request("unknown route");
}

session::session(tcp::socket&& socket,
                 EntityManager& manager,
                 std::vector<std::string> caps)
    : stream_{std::move(socket)},
      manager_{manager},
      capabilities_{std::move(caps)} {}

void session::start() {
    net::dispatch(
        stream_.get_executor(),
        beast::bind_front_handler(&session::do_read, shared_from_this()));
}

void session::do_read() {
    request_ = {};
    http::async_read(
        stream_, buffer_, request_,
        beast::bind_front_handler(&session::on_read, shared_from_this()));
}

void session::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec == http::error::end_of_stream) {
        return do_close();
    }
    if (ec) {
        std::cout << "run failed\n";
        return;
    }
    send_response(handle_request(std::move(request_)));
}

void session::do_close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void session::send_response(http::message_generator&& msg) {
    beast::async_write(
        stream_, std::move(msg),
        beast::bind_front_handler(&session::on_write, shared_from_this(),
                                  request_.keep_alive()));
}

void session::on_write(bool keep_alive,
                       beast::error_code ec,
                       std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cout << "write failed\n";
        return;
    }

    if (!keep_alive) {
        return do_close();
    }

    do_read();
}
