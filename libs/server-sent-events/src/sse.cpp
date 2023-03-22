#include <boost/asio/placeholders.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/optional/optional.hpp>
#include <boost/url/parse.hpp>
#include <iostream>
#include <launchdarkly/sse/sse.hpp>
#include <memory>
#include <tuple>

namespace launchdarkly::sse {

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;  // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

event_data::event_data() : m_type{}, m_data{}, m_id{} {}

void event_data::set_type(std::string type) {
    m_type = std::move(type);
}
void event_data::append_data(std::string const& data) {
    m_data.append(data);
    m_data.append("\n");
}

void event_data::set_id(std::optional<std::string> id) {
    m_id = std::move(id);
}

std::string const& event_data::get_type() {
    return m_type;
}
std::string const& event_data::get_data() {
    return m_data;
}

void event_data::trim_trailing_newline() {
    if (m_data[m_data.size() - 1] == '\n') {
        m_data.resize(m_data.size() - 1);
    }
}

std::optional<std::string> const& event_data::get_id() {
    return m_id;
}

client::client(net::any_io_executor ex,
               http::request<http::empty_body> req,
               std::string host,
               std::string port)
    : m_resolver{ex},
      parser_{},
      host_{std::move(host)},
      port_{std::move(port)},
      m_request{std::move(req)},
      buffered_line_{},
      complete_lines_{},
      begin_CR_{false},
      last_event_id_{},
      m_event_data{},
      m_events{},
      on_chunk_body_trampoline_{},
      m_cb{[](event_data e) {
          std::cout << "Event[" << e.get_type() << "] = <" << e.get_data()
                    << ">\n";
      }} {}

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

std::pair<std::string, std::string> parse_field(std::string field) {
    if (field.empty()) {
        assert(0 && "should never parse an empty line");
    }

    size_t colon_index = field.find(':');
    switch (colon_index) {
        case 0:
            field.erase(0, 1);
            return std::make_pair(std::string{"comment"}, std::move(field));
        case std::string::npos:
            return std::make_pair(std::move(field), std::string{});
        default:
            auto key = field.substr(0, colon_index);
            field.erase(0, colon_index + 1);
            if (field.find(' ') == 0) {
                field.erase(0, 1);
            }
            return std::make_pair(std::move(key), std::move(field));
    }
}

void client::parse_events() {
    while (true) {
        bool seen_empty_line = false;

        while (!complete_lines_.empty()) {
            std::string line = std::move(complete_lines_.front());
            complete_lines_.pop_front();

            if (line.empty()) {
                if (m_event_data.has_value()) {
                    seen_empty_line = true;
                    break;
                }
                continue;
            }

            auto field = parse_field(std::move(line));
            if (field.first == "comment") {
                event_data e;
                e.set_type("comment");
                e.append_data(field.second);
                m_cb(std::move(e));
                continue;
            }

            if (!m_event_data.has_value()) {
                m_event_data.emplace(event_data{});
                m_event_data->set_id(last_event_id_);
            }

            if (field.first == "event") {
                m_event_data->set_type(field.second);
            } else if (field.first == "data") {
                m_event_data->append_data(field.second);
            } else if (field.first == "id") {
                if (field.second.find('\0') != std::string::npos) {
                    std::cout
                        << "Debug: ignoring event ID will null terminator\n";
                    continue;
                }
                last_event_id_ = field.second;
                m_event_data->set_id(last_event_id_);
            } else if (field.first == "retry") {
                std::cout << "Got RETRY field\n";
            }
        }

        if (seen_empty_line) {
            std::optional<event_data> data = m_event_data;
            m_event_data = std::nullopt;

            if (data.has_value()) {
                data->trim_trailing_newline();
                m_cb(std::move(*data));
            }

            continue;
        }

        break;
    }
}

void client::complete_line() {
    if (buffered_line_.has_value()) {
        complete_lines_.push_back(buffered_line_.value());
        buffered_line_.reset();
    }
}

size_t client::append_up_to(boost::string_view body,
                            std::string const& search) {
    std::size_t index = body.find_first_of(search);
    if (index != std::string::npos) {
        body.remove_suffix(body.size() - index);
    }
    if (buffered_line_.has_value()) {
        buffered_line_->append(body.to_string());
    } else {
        buffered_line_ = std::string{body};
    }
    return index == std::string::npos ? body.size() : index;
}

size_t client::parse_stream(std::uint64_t remain,
                            boost::string_view body,
                            beast::error_code& ec) {
    size_t i = 0;
    while (i < body.length()) {
        i += this->append_up_to(body.substr(i, body.length() - i), "\r\n");
        if (i == body.size()) {
            continue;
        } else if (body.at(i) == '\r') {
            if (this->begin_CR_) {
                assert(0 && "illegal carriage return (likely a bug in the parser)");
            } else {
                this->begin_CR_ = true;
                i++;
            }
        } else if (body.at(i) == '\n') {
            this->begin_CR_ = false;
            this->complete_line();
            i++;
        }
    }
    return body.length();
}

class ssl_client : public client {
    beast::ssl_stream<beast::tcp_stream> stream_;

    std::shared_ptr<ssl_client> shared() {
        return std::static_pointer_cast<ssl_client>(shared_from_this());
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&ssl_client::on_connect, shared()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        stream_.async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(&ssl_client::on_handshake, shared()));
    }

    void on_handshake(beast::error_code ec) {
        if (ec)
            return fail(ec, "handshake");

        // Send the HTTP request to the remote host
        http::async_write(
            stream_, m_request,
            beast::bind_front_handler(&ssl_client::on_write, shared()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(10));

        on_chunk_body_trampoline_.emplace(
            [self = shared()](auto remain, auto body, auto ec) {
                auto consumed = self->parse_stream(remain, body, ec);
                self->parse_events();
                return consumed;
            });

        parser_.on_chunk_body(*this->on_chunk_body_trampoline_);

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&ssl_client::on_read, shared()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return fail(ec, "read");
        }

        beast::get_lowest_layer(stream_).expires_never();

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&ssl_client::on_read, shared()));
    }

    void on_stop() {
        beast::error_code ec;
        beast::close_socket(beast::get_lowest_layer(stream_));
    }

   public:
    ssl_client(net::any_io_executor ex,
               ssl::context& ctx,
               http::request<http::empty_body> req,
               std::string host,
               std::string port)
        : client(ex, std::move(req), std::move(host), std::move(port)),
          stream_{ex, ctx} {}

    void read() override {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        m_resolver.async_resolve(
            host_, port_,
            beast::bind_front_handler(&ssl_client::on_resolve, shared()));
    }

    void close() override {
        net::post(
            stream_.get_executor(),
            beast::bind_front_handler(
                &ssl_client::on_stop,
                shared()));
    }
};

class plaintext_client : public client {
    beast::tcp_stream stream_;

    std::shared_ptr<plaintext_client> shared() {
        return std::static_pointer_cast<plaintext_client>(shared_from_this());
    }

    void on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
        if (ec)
            return fail(ec, "resolve");
        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream_).async_connect(
            results,
            beast::bind_front_handler(&plaintext_client::on_connect, shared()));
    }

    void on_connect(beast::error_code ec,
                    tcp::resolver::results_type::endpoint_type) {
        if (ec)
            return fail(ec, "connect");

        http::async_write(
            stream_, m_request,
            beast::bind_front_handler(&plaintext_client::on_write, shared()));
    }

    void on_write(beast::error_code ec, std::size_t) {
        if (ec)
            return fail(ec, "write");

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(10));

        on_chunk_body_trampoline_.emplace(
            [self = shared()](auto remain, auto body, auto ec) {
                auto consumed = self->parse_stream(remain, body, ec);
                self->parse_events();
                return consumed;
            });

        parser_.on_chunk_body(*this->on_chunk_body_trampoline_);

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec) {
            return fail(ec, "read");
        }

        beast::get_lowest_layer(stream_).expires_never();

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_stop() {
        beast::error_code ec;
        beast::close_socket(beast::get_lowest_layer(stream_));
    }

   public:
    plaintext_client(net::any_io_executor ex,
                     ssl::context& ctx,
                     http::request<http::empty_body> req,
                     std::string host,
                     std::string port)
        : client(ex, std::move(req), std::move(host), std::move(port)),
          stream_{ex} {}

    void read() override {
        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        m_resolver.async_resolve(
            host_, port_,
            beast::bind_front_handler(&plaintext_client::on_resolve, shared()));
    }

    void close() override {
        net::post(
            stream_.get_executor(),
            beast::bind_front_handler(
                &plaintext_client::on_stop,
                shared()));
    }
};

builder::builder(net::any_io_executor ctx, std::string url)
    : m_url{std::move(url)},
      m_ssl_ctx{ssl::context::tlsv12_client},
      m_executor{std::move(ctx)},
      tls_version_{12} {
    // This needs to be verify_peer in production!!
    m_ssl_ctx.set_verify_mode(ssl::verify_none);

    m_request.version(11);
    m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    m_request.method(http::verb::get);
    m_request.set("Accept", "text/event-stream");
    m_request.set("Cache-Control", "no-cache");
}

builder& builder::header(std::string const& name, std::string const& value) {
    m_request.set(name, value);
    return *this;
}

builder& builder::method(http::verb verb) {
    m_request.method(verb);
    return *this;
}

builder& builder::tls(ssl::context_base::method ctx) {
    m_ssl_ctx = ssl::context{ctx};
    return *this;
}

std::shared_ptr<client> builder::build() {
    boost::system::result<boost::urls::url_view> uri_components =
        boost::urls::parse_uri(m_url);
    if (!uri_components) {
        return nullptr;
    }

    m_request.set(http::field::host, uri_components->host());
    m_request.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        return std::make_shared<ssl_client>(net::make_strand(m_executor),
                                            m_ssl_ctx, m_request,
                                            uri_components->host(), port);
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<plaintext_client>(net::make_strand(m_executor),
                                                  m_ssl_ctx, m_request,
                                                  uri_components->host(), port);
    }
}

}  // namespace launchdarkly::sse
