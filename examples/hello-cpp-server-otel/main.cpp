#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <random>
#include <cstring>

#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>

#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/server_side/client.hpp>
#include <launchdarkly/server_side/config/config_builder.hpp>

#include <launchdarkly/server_side/integrations/otel/tracing_hook.hpp>

// Set SDK_KEY to your LaunchDarkly SDK key.
#define SDK_KEY ""

// Set FEATURE_FLAG_KEY to the feature flag key you want to evaluate.
#define FEATURE_FLAG_KEY "show-detailed-weather"

// Set INIT_TIMEOUT_MILLISECONDS to the amount of time you will wait for
// the client to become initialized.
#define INIT_TIMEOUT_MILLISECONDS 3000

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg);

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace nostd = opentelemetry::nostd;

// Initialize OpenTelemetry
void InitTracer(char const* sdk_key) {
    opentelemetry::exporter::otlp::OtlpHttpExporterOptions opts;

    // Check for custom endpoint from environment variable
    if (char const* custom_endpoint = std::getenv("LD_OTEL_ENDPOINT");
        custom_endpoint && strlen(custom_endpoint)) {
        opts.url = std::string(custom_endpoint);
    } else {
        opts.url = "https://otel.observability.app.launchdarkly.com:4318/v1/traces";
    }

    // Create resource with highlight.project_id attribute
    auto resource_attributes = opentelemetry::sdk::resource::ResourceAttributes{
        {"highlight.project_id", sdk_key}
    };
    auto resource = opentelemetry::sdk::resource::Resource::Create(resource_attributes);

    auto exporter =
        opentelemetry::exporter::otlp::OtlpHttpExporterFactory::Create(opts);
    auto processor = trace_sdk::SimpleSpanProcessorFactory::Create(
        std::move(exporter));
    const std::shared_ptr<trace_api::TracerProvider> provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor), resource);
    trace_api::Provider::SetTracerProvider(provider);
}

// Get tracer
nostd::shared_ptr<trace_api::Tracer> get_tracer() {
    const auto provider = trace_api::Provider::GetTracerProvider();
    return provider->GetTracer("weather-server", "1.0.0");
}

// Random weather generator
std::string get_random_weather() {
    static std::vector<std::string> weather_conditions = {
        "Sunny",
        "Cloudy",
        "Rainy",
        "Snowy",
        "Windy",
        "Foggy",
        "Stormy",
        "Partly Cloudy"
    };

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<>
        dis(0, weather_conditions.size() - 1);

    return weather_conditions[dis(gen)];
}

// Handle HTTP request
http::response<http::string_body> handle_request(
    http::request<http::string_body>&& req,
    std::shared_ptr<launchdarkly::server_side::Client> ld_client) {
    auto tracer = get_tracer();

    // Start a span for the HTTP request
    auto span = tracer->StartSpan(
        "HTTP " + std::string(req.method_string()) + " " + std::string(
            req.target()));
    auto scope = trace_api::Scope(span);

    // Add HTTP attributes to the span
    span->SetAttribute("http.method", std::string(req.method_string()));
    span->SetAttribute("http.target", std::string(req.target()));
    span->SetAttribute("http.scheme", "http");

    http::response<http::string_body> res;

    if (req.target() == "/weather") {
        auto context = launchdarkly::ContextBuilder()
                       .Kind("user", "weather-api-user")
                       .Name("Weather API User")
                       .Build();

        // When using an async framework you need to manually specify the current span.
        // With a threaded framework the active span can be accessed automatically.
        auto hook_ctx =
            launchdarkly::server_side::integrations::otel::MakeHookContextWithSpan(
                span);

        // Pass the HookContext to the evaluation
        auto show_detailed_weather = ld_client->BoolVariation(
            context, FEATURE_FLAG_KEY, false, hook_ctx);

        std::string weather = get_random_weather();
        span->SetAttribute("weather.condition", weather);

        res.result(http::status::ok);
        res.set(http::field::content_type, "text/plain");

        if (show_detailed_weather) {
            res.body() = "Current weather: " + weather +
                         " (detailed mode enabled via LaunchDarkly flag)";
        } else {
            res.body() = "Current weather: " + weather;
        }

        span->SetAttribute("http.status_code", 200);
    } else {
        res.result(http::status::not_found);
        res.set(http::field::content_type, "text/plain");
        res.body() = "404 Not Found";

        span->SetAttribute("http.status_code", 404);
    }

    res.version(req.version());
    res.keep_alive(req.keep_alive());
    res.prepare_payload();

    span->End();

    return res;
}

// Session handles a single connection
class session : public std::enable_shared_from_this<session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::shared_ptr<launchdarkly::server_side::Client> ld_client_;

public:
    explicit session(tcp::socket socket,
                     const std::shared_ptr<launchdarkly::server_side::Client>
                     & ld_client)
        : socket_(std::move(socket)), ld_client_(ld_client) {
    }

    void run() {
        do_read();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
                         [self](const beast::error_code& ec, std::size_t) {
                             if (!ec) {
                                 self->do_write();
                             }
                         });
    }

    void do_write() {
        auto self = shared_from_this();
        auto res = std::make_shared<http::response<http::string_body>>(
            handle_request(std::move(req_), ld_client_));

        http::async_write(socket_, *res,
                          [self, res](beast::error_code ec, std::size_t) {
                              self->socket_.shutdown(
                                  tcp::socket::shutdown_send, ec);
                          });
    }
};

// Listener accepts incoming connections
class listener : public std::enable_shared_from_this<listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<launchdarkly::server_side::Client> ld_client_;

public:
    listener(net::io_context& ioc,
             const tcp::endpoint& endpoint,
             const std::shared_ptr<launchdarkly::server_side::Client>& ld_client)
        : ioc_(ioc)
          , acceptor_(ioc)
          , ld_client_(ld_client) {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            std::cerr << "open: " << ec.message() << std::endl;
            return;
        }

        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            std::cerr << "set_option: " << ec.message() << std::endl;
            return;
        }

        acceptor_.bind(endpoint, ec);
        if (ec) {
            std::cerr << "bind: " << ec.message() << std::endl;
            return;
        }

        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            std::cerr << "listen: " << ec.message() << std::endl;
            return;
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [self = shared_from_this()](const beast::error_code& ec,
                                        tcp::socket socket) {
                if (!ec) {
                    std::make_shared<session>(std::move(socket),
                                              self->ld_client_)->run();
                }
                self->do_accept();
            });
    }
};

int main() {
    // Initialize LaunchDarkly
    char const* sdk_key = get_with_env_fallback(
        SDK_KEY, "LD_SDK_KEY",
        "Please edit main.cpp to set SDK_KEY to your LaunchDarkly server-side "
        "SDK key first.\n\nAlternatively, set the LD_SDK_KEY environment "
        "variable.\n"
        "The value of SDK_KEY in main.cpp takes priority over LD_SDK_KEY.");

    // Initialize OpenTelemetry
    InitTracer(sdk_key);

    // Create the OpenTelemetry tracing hook using builder pattern
    auto hook_options =
        launchdarkly::server_side::integrations::otel::TracingHookOptionsBuilder()
        .IncludeValue(true) // Include flag values in traces
        .CreateSpans(false) // Only create span events, not full spans
        .Build();
    auto tracing_hook = std::make_shared<
        launchdarkly::server_side::integrations::otel::TracingHook>(
        hook_options);

    auto config = launchdarkly::server_side::ConfigBuilder(sdk_key)
                  .Hooks(tracing_hook)
                  .Build();
    if (!config) {
        std::cerr << "*** LaunchDarkly config is invalid: " << config.error() <<
            std::endl;
        return EXIT_FAILURE;
    }

    auto ld_client = std::make_shared<launchdarkly::server_side::Client>(
        std::move(*config));

    auto start_result = ld_client->StartAsync();

    if (auto const status = start_result.wait_for(
            std::chrono::milliseconds(INIT_TIMEOUT_MILLISECONDS));
        status == std::future_status::ready) {
        if (start_result.get()) {
            std::cout << "*** SDK successfully initialized!\n\n";
        } else {
            std::cerr << "*** SDK failed to initialize\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "*** SDK initialization didn't complete in "
            << INIT_TIMEOUT_MILLISECONDS << "ms\n";
        return EXIT_FAILURE;
    }

    try {
        auto const address = net::ip::make_address("0.0.0.0");
        constexpr auto port = static_cast<unsigned short>(8080);

        net::io_context ioc{1};

        std::make_shared<listener>(ioc, tcp::endpoint{address, port}, ld_client)
            ->run();

        std::cout << "*** Weather server running on http://0.0.0.0:8080\n";
        std::cout << "*** Try: curl http://localhost:8080/weather\n";
        std::cout <<
            "*** OpenTelemetry tracing enabled, sending traces to LaunchDarkly\n";
        std::cout <<
            "*** LaunchDarkly integration enabled with OpenTelemetry tracing hook\n\n";

        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "*** Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

char const* get_with_env_fallback(char const* source_val,
                                  char const* env_variable,
                                  char const* error_msg) {
    if (strlen(source_val)) {
        return source_val;
    }

    if (char const* from_env = std::getenv(env_variable);
        from_env && strlen(from_env)) {
        return from_env;
    }

    std::cout << "*** " << error_msg << std::endl;
    std::exit(1);
}