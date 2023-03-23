#include <launchdarkly/sse/client.hpp>

#include <boost/asio/strand.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/optional/optional.hpp>
#include <boost/url/parse.hpp>
#include <iostream>
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
               std::string port,
               std::function<void(std::string)> logging_cb,
               std::string log_tag)
    : m_resolver{std::move(ex)},
      parser_{},
      host_{std::move(host)},
      port_{std::move(port)},
      m_request{std::move(req)},
      m_response{},
      buffered_line_{},
      complete_lines_{},
      begin_CR_{false},
      last_event_id_{},
      m_event_data{},
      m_events{},
      logging_cb_{std::move(logging_cb)},
      on_chunk_body_trampoline_{},
      log_tag_{std::move(log_tag)},
      m_cb{[this](event_data e) {
          log("got event: (" + e.get_type() + ", " + e.get_data() + ")");
      }} {
    parser_.body_limit(boost::none);
    log("create");
}

client::~client() {
    log("destroy");
}

void client::log(std::string what) {
    if (logging_cb_) {
        logging_cb_(log_tag_ + ": " + std::move(what));
    }
}

// Report a failure
void client::fail(beast::error_code ec, char const* what) {
    log(std::string(what) + ":" + ec.message());
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
                    // IDs with null-terminators are acceptable, but ignored.
                    continue;
                }
                last_event_id_ = field.second;
                m_event_data->set_id(last_event_id_);
            } else if (field.first == "retry") {
                log("got unhandled 'retry' field");
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
            complete_line();
            begin_CR_ = true;
            i++;
        } else if (body.at(i) == '\n') {
            if (begin_CR_) {
                begin_CR_ = false;
                i++;
            } else {
                complete_line();
                i++;
            }
        } else {
            begin_CR_ = false;
        }
    }
    return body.length();
}

class ssl_client : public client {
   public:
    ssl_client(net::any_io_executor ex,
               ssl::context& ctx,
               http::request<http::empty_body> req,
               std::string host,
               std::string port,
               logger logging_cb)
        : client(ex,
                 std::move(req),
                 std::move(host),
                 std::move(port),
                 std::move(logging_cb),
                 "sse-tls"),
          stream_{ex, ctx} {}

    void run() override {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
            beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                 net::error::get_ssl_category()};
            log("failed to set TLS host name extension: " + ec.message());
            return;
        }

        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        m_resolver.async_resolve(
            host_, port_,
            beast::bind_front_handler(&ssl_client::on_resolve, shared()));
    }

    void close() override {
        net::post(stream_.get_executor(),
                  beast::bind_front_handler(&ssl_client::on_stop, shared()));
    }

   private:
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

    void on_stop() { beast::get_lowest_layer(stream_).cancel(); }
};

class plaintext_client : public client {
   public:
    plaintext_client(net::any_io_executor ex,
                     ssl::context& ctx,
                     http::request<http::empty_body> req,
                     std::string host,
                     std::string port,
                     logger logger)
        : client(ex,
                 std::move(req),
                 std::move(host),
                 std::move(port),
                 std::move(logger),
                 "sse-plaintext"),
          stream_{ex},
          ctx_{ctx} {}

    void run() override {
        beast::get_lowest_layer(stream_).expires_after(
            std::chrono::seconds(15));

        m_resolver.async_resolve(
            host_, port_,
            beast::bind_front_handler(&plaintext_client::on_resolve, shared()));
    }

    void close() override {
        net::post(
            stream_.get_executor(),
            beast::bind_front_handler(&plaintext_client::on_stop, shared()));
    }

   private:
    beast::tcp_stream stream_;
    ssl::context& ctx_;

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

        http::async_read(stream_, m_buffer, parser_,
                         beast::bind_front_handler(
                             &plaintext_client::on_got_headers, shared()));
    }

    void on_got_headers(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            return fail(ec, "headers");
        }

        if (parser_.is_header_done()) {
            auto status = parser_.get().result();
            if (status == http::status::moved_permanently) {
                if (auto it = parser_.get().find("Location");
                    it != parser_.get().end()) {
                }
            }
        }

        beast::get_lowest_layer(stream_).expires_never();

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if (ec && ec != beast::errc::operation_canceled) {
            return fail(ec, "read");
        }

        http::async_read_some(
            stream_, m_buffer, parser_,
            beast::bind_front_handler(&plaintext_client::on_read, shared()));
    }

    void on_stop() { stream_.cancel(); }
};

builder::builder(net::any_io_executor ctx, std::string url)
    : url_{std::move(url)},
      ssl_context_{ssl::context::tlsv12_client},
      executor_{std::move(ctx)},
      tls_version_{12} {
    // This needs to be verify_peer in production!!
    ssl_context_.set_verify_mode(ssl::verify_none);

    request_.version(11);
    request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    request_.method(http::verb::get);
    request_.set("Accept", "text/event-stream");
    request_.set("Cache-Control", "no-cache");
}

builder& builder::header(std::string const& name, std::string const& value) {
    request_.set(name, value);
    return *this;
}

builder& builder::method(http::verb verb) {
    request_.method(verb);
    return *this;
}

builder& builder::tls(ssl::context_base::method ctx) {
    ssl_context_ = ssl::context{ctx};
    return *this;
}

builder& builder::logging(std::function<void(std::string)> cb) {
    logging_cb_ = std::move(cb);
    return *this;
}

std::shared_ptr<client> builder::build() {
    boost::system::result<boost::urls::url_view> uri_components =
        boost::urls::parse_uri(url_);
    if (!uri_components) {
        return nullptr;
    }

    request_.set(http::field::host, uri_components->host());
    request_.target(uri_components->path());

    if (uri_components->scheme_id() == boost::urls::scheme::https) {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "443";

        return std::make_shared<ssl_client>(
            net::make_strand(executor_), ssl_context_, request_,
            uri_components->host(), port, logging_cb_);
    } else {
        std::string port =
            uri_components->has_port() ? uri_components->port() : "80";

        return std::make_shared<plaintext_client>(
            net::make_strand(executor_), ssl_context_, request_,
            uri_components->host(), port, logging_cb_);
    }
}

}  // namespace launchdarkly::sse
