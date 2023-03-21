#pragma once

#include "entity_manager.hpp"
#include <nlohmann/json.hpp>
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




class session : public std::enable_shared_from_this<session>
{
public:
    explicit session(tcp::socket socket, entity_manager& manager, std::vector<std::string> caps):
            socket_{std::move(socket)},
            manager_{manager},
            capabilities_{std::move(caps)}
    {
    }

    void start() {
        read_request();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::string_body> request_;

    // The response message.
    http::response<http::string_body> response_;

    entity_manager& manager_;

    std::vector<std::string> capabilities_;



    // Asynchronously receive a complete request message.
    void
    read_request()
    {
        auto self = shared_from_this();

        http::async_read(
                socket_,
                buffer_,
                request_,
                [self](beast::error_code ec,
                       std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);
                    if(!ec) {
                        self->process_request();
                    }
                });
    }

    // Determine what needs to be done with the request message.
    void
    process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        if (create_response()) {
            write_response();
        } else {
            socket_.shutdown(boost::asio::socket_base::shutdown_both);
            socket_.close();
            std::raise(SIGTERM);
        }
    }

    // Construct a response message based on the program state.
    bool
    create_response()
    {
        if (request_.target() == "/") {
            if (request_.method() == http::verb::get) {
                response_.set(http::field::content_type, "application/json");
                response_.result(http::status::ok);

                nlohmann::json status {
                    {"capabilities", capabilities_}
                };
                response_.body() = status.dump();
                return true;
            }
            else if (request_.method() == http::verb::delete_) {
                return false;
            } else if (request_.method() == http::verb::post) {
                auto json = nlohmann::json::parse(request_.body());
                auto params = json.get<config_params>();
                std::string id = manager_.create(params);
                response_.result(http::status::ok);
                response_.set("Location", "/shutdown/" + id);
                std::cout << "creating entity " << id << '\n';
                return true;
            }
        } else if (request_.target().starts_with("/shutdown/")) {
            auto id = request_.target();
            id.remove_prefix(std::strlen("/shutdown/"));
            std::cout << "<"<< id << ">" << std::endl;
            if (manager_.destroy(id)) {
                response_.result(http::status::ok);
            } else {
                response_.result(http::status::not_found);
            }
            return true;
        }

        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        return true;
    }
    void
    write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t bytes_written)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                });
    }
};
