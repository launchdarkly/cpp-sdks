#include <launchdarkly/sse/sse.hpp>
#include <iostream>
#include <boost/url/parse.hpp>
#include <boost/optional/optional.hpp>
#include <boost/asio/placeholders.hpp>
#include <memory>
#include <tuple>
#include <boost/beast/websocket.hpp>

namespace launchdarkly::sse {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

builder::builder(net::any_io_executor ctx, std::string url):
    m_url{std::move(url)},
    m_ssl_ctx{ssl::context::tlsv12_client},
    m_executor{ctx} {

    // This needs to be verify_peer in production!!
    m_ssl_ctx.set_verify_mode(ssl::verify_none);

    m_request.version(11);
    m_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    m_request.method(http::verb::get);
    m_request.set("Accept", "text/event-stream");
    m_request.set("Cache-Control", "no-cache");
}

builder& builder::header(const std::string& name, const std::string& value) {
    m_request.set(name, value);
    return *this;
}

builder& builder::method(http::verb verb) {
    m_request.method(verb);
    return *this;
}


std::shared_ptr<client> builder::build() {
    boost::system::result<boost::urls::url_view> uri_components = boost::urls::parse_uri(m_url);
    if (!uri_components) {
        return nullptr;
    }
    std::string port;
    if (!uri_components->has_port() && uri_components->scheme_id() == boost::urls::scheme::https) {
        port = "443";
    }

    m_request.set(http::field::host, uri_components->host());
    m_request.target(uri_components->path());

   return std::make_shared<client>(
            net::make_strand(m_executor),
            m_ssl_ctx,
            m_request,
            uri_components->host(),
            port
    );
}


event_data::event_data(boost::optional<std::string> id):
    m_type{},
    m_data{},
    m_id{std::move(id)} {}

void event_data::set_type(std::string type) {
    m_type = std::move(type);
}
void event_data::append_data(std::string const& data)  {
    m_data.append(data);
}

std::string const& event_data::get_type() {
    return m_type;
}
std::string const& event_data::get_data() {
    return m_data;
}

client::client(net::any_io_executor ex, ssl::context &ctx, http::request<http::empty_body> req, std::string host, std::string port):
        m_resolver{ex},
        m_stream{ex, ctx},
        m_request{std::move(req)},
        m_host{std::move(host)},
        m_port{std::move(port)},
        m_parser{},
        m_buffered_line{},
        m_complete_lines{},
        m_begin_CR{false},
        m_event_data{},
        m_events{},
        m_cb{[](event_data e){
            std::cout << "Event[" << e.get_type() << "] = <" << e.get_data() << ">\n";
        }}{

    // The HTTP response body is of potentially infinite length.
    m_parser.body_limit(boost::none);
}



void client::read() {
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(m_stream.native_handle(), m_host.c_str()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        std::cerr << ec.message() << "\n";
        return;
    }


    beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(10));

    auto results = m_resolver.resolve(m_host, m_port);
    beast::get_lowest_layer(m_stream).connect(results);
    m_stream.handshake(ssl::stream_base::client);
    http::write(m_stream, m_request);

    beast::get_lowest_layer(m_stream).expires_never();

    auto callback =
            [this](std::uint64_t remain, std::string_view body, beast::error_code& ec)
            {
                size_t read = this->parse_stream(remain, body, ec);
                this->parse_events();
                return read;
            };

    m_parser.on_chunk_body(callback);

    // Blocking call until the stream terminates.
    http::read(m_stream, m_buffer, m_parser);
}


void client::complete_line() {
    if (m_buffered_line.has_value()) {
        m_complete_lines.push_back(m_buffered_line.value());
        std::cout << "Line: <" << m_buffered_line.value() << ">" << std::endl;
        m_buffered_line.reset();
    }
}


size_t client::append_up_to(std::string_view body, const std::string& search) {
    std::size_t index = body.find_first_of(search);
    if(index != std::string::npos) {
        body.remove_suffix(body.size() - index);
    }
    if (m_buffered_line.has_value()) {
        m_buffered_line->append(body);
    } else {
        m_buffered_line = std::string{body};
    }
    return index == std::string::npos ? body.size() : index;
}



std::size_t client::parse_stream(std::uint64_t remain, std::string_view body, beast::error_code &ec) {
    size_t i = 0;
    while (i < body.length()) {
        i += this->append_up_to(body.substr(i, body.length()-i), "\r\n");
        if (body[i] =='\r') {
            if (this->m_begin_CR) {
                //todo: illegal token
            } else {
                this->m_begin_CR = true;
            }
        } else if (body[i] == '\n') {
            this->m_begin_CR = false;
            this->complete_line();
            i++;
        }
    }
    return body.length();
}

boost::optional<std::pair<std::string, std::string>> parse_field(std::string field){

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
    while(true) {
        bool seen_empty_line = false;

        while (!m_complete_lines.empty()) {
            std::string line = std::move(m_complete_lines.front());
            m_complete_lines.pop_front();

            if (line.empty()) {
                if (m_event_data.has_value()) {
                    seen_empty_line = true;
                    break;
                }
                continue;
            }

            if (auto field = parse_field(std::move(line))) {

                if (field->first == "comment") {
                    event_data e{boost::none};
                    e.set_type("comment");
                    e.append_data(field->second);
                    m_cb(std::move(e));
                    continue;
                }

                if (!m_event_data.has_value()) {
                    m_event_data.emplace(event_data{boost::none});
                }

                if (field->first == "event") {
                    m_event_data->set_type(field->second);
                } else if (field->first == "data") {
                    m_event_data->append_data(field->second);
                } else if (field->first == "id") {
                    std::cout << "Got ID field\n";
                } else if (field->first == "retry") {
                    std::cout << "Got RETRY field\n";
                }
            }
        }

        if (seen_empty_line) {
            boost::optional<event_data> data = m_event_data;
            m_event_data.reset();

            if (data.has_value()) {
                m_cb(std::move(data.get()));
            }

            continue;
        }

        break;
    }
}

} // namespace launchdarkly
