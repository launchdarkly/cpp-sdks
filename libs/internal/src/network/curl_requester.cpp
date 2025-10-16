#ifdef LD_CURL_NETWORKING

#include "launchdarkly/network/curl_requester.hpp"
#include <curl/curl.h>
#include <memory>

namespace launchdarkly::network {

// Custom HTTP method strings
static constexpr auto const* kHttpMethodPut = "PUT";
static constexpr auto const* kHttpMethodReport = "REPORT";

// Header parsing constants
static constexpr auto kHttpPrefix = "HTTP/";
static constexpr auto const* kCrLf = "\r\n";
static constexpr auto const* kLf = "\n";
static constexpr auto const* kWhitespace = " \t";
static constexpr auto const* kWhitespaceWithNewlines = " \t\r\n";
static constexpr auto const* kHeaderSeparator = ": ";

// Error messages
static constexpr auto const* kErrorMalformedRequest = "The request was malformed and could not be made.";
static constexpr auto const* kErrorCurlInit = "Failed to initialize CURL";
static constexpr auto const* kErrorHeaderAppend = "Failed to append headers to CURL";
static constexpr auto const* kErrorCurlPrefix = "CURL error: ";

// Callback for writing response data
//
// https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
// Our userdata is a std::string which we accumulate the body in.
static size_t WriteCallback(void* contents, const size_t size, const size_t dataSize, void* userdata) {
    const size_t total_size = size * dataSize;
    const auto stringData = static_cast<std::string*>(userdata);
    stringData->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// Callback for reading request headers
//
// https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
// Our user data is our HttpResult::HeadersType that we populate with
// headers as we receive them.
static size_t HeaderCallback(const char* buffer, const size_t size, const size_t dataSize, void* userdata) {
    const size_t total_size = size * dataSize;
    auto* headers = static_cast<HttpResult::HeadersType*>(userdata);

    std::string header(buffer, total_size);

    // Skip status line and empty lines
    if (header.find(kHttpPrefix) == 0 || header == kCrLf || header == kLf) {
        return total_size;
    }

    // Parse header
    if (const size_t colon_pos = header.find(':'); colon_pos != std::string::npos) {
        const std::string key = header.substr(0, colon_pos);
        std::string value = header.substr(colon_pos + 1);

        // Trim whitespace
        value.erase(0, value.find_first_not_of(kWhitespace));
        value.erase(value.find_last_not_of(kWhitespaceWithNewlines) + 1);

        headers->insert_or_assign(key, value);
    }

    return total_size;
}

CurlRequester::CurlRequester(net::any_io_executor ctx, TlsOptions const& tls_options)
    : ctx_(std::move(ctx)),
      tls_options_(tls_options),
      multi_manager_(CurlMultiManager::create(ctx_)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void CurlRequester::Request(HttpRequest request, std::function<void(const HttpResult&)> cb) const {
    // Copy necessary data to avoid capturing 'this'
    auto multi_manager = multi_manager_;
    auto tls_options = tls_options_;

    boost::asio::post(ctx_, [multi_manager, tls_options, request = std::move(request), cb = std::move(cb)]() mutable {
        PerformRequestWithMulti(multi_manager, tls_options, std::move(request), std::move(cb));
    });
}

void CurlRequester::PerformRequestWithMulti(std::shared_ptr<CurlMultiManager> multi_manager,
                                             TlsOptions const& tls_options,
                                             const HttpRequest& request,
                                             std::function<void(const HttpResult&)> cb) {
    // Validate request
    if (!request.Valid()) {
        cb(HttpResult(kErrorMalformedRequest));
        return;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        cb(HttpResult(kErrorCurlInit));
        return;
    }

    // Create context to hold data for this request
    // This will be cleaned up in the completion callback
    struct RequestContext {
        CURL* curl;
        std::string url;
        std::string body; // Keep body alive
        std::string response_body;
        HttpResult::HeadersType response_headers;
        std::function<void(const HttpResult&)> callback;

        ~RequestContext() {
            // Headers are managed by CurlMultiManager
            if (curl) {
                curl_easy_cleanup(curl);
            }
        }
    };

    auto ctx = std::make_shared<RequestContext>();
    ctx->curl = curl;
    ctx->callback = std::move(cb);

    // Headers will be managed by CurlMultiManager
    curl_slist* headers = nullptr;

    // Helper macro to check curl_easy_setopt return values
    #define CURL_SETOPT_CHECK(handle, option, parameter) \
        do { \
            CURLcode code = curl_easy_setopt(handle, option, parameter); \
            if (code != CURLE_OK) { \
                std::string error_message = kErrorCurlPrefix; \
                error_message += "curl_easy_setopt failed for " #option ": "; \
                error_message += curl_easy_strerror(code); \
                if (headers) { \
                    curl_slist_free_all(headers); \
                } \
                ctx->callback(HttpResult(error_message)); \
                return; \
            } \
        } while(0)

    // Store URL to keep it alive for the duration of the request
    ctx->url = request.Url();

    // Set URL
    CURL_SETOPT_CHECK(curl, CURLOPT_URL, ctx->url.c_str());

    // Set HTTP method
    if (request.Method() == HttpMethod::kPost) {
        // Basically CURLOPT_POST is a flag that indicates this is a post.
        // Passing 1 enables this flag.
        // This will also set a content type, but the headers for the request
        // should override that with the correct value.
        CURL_SETOPT_CHECK(curl, CURLOPT_POST, 1L);
    } else if (request.Method() == HttpMethod::kPut) {
        CURL_SETOPT_CHECK(curl, CURLOPT_CUSTOMREQUEST, kHttpMethodPut);
    } else if (request.Method() == HttpMethod::kReport) {
        CURL_SETOPT_CHECK(curl, CURLOPT_CUSTOMREQUEST, kHttpMethodReport);
    } else if (request.Method() == HttpMethod::kGet) {
        CURL_SETOPT_CHECK(curl, CURLOPT_HTTPGET, 1L);
    }

    // Set request body if present
    if (request.Body().has_value()) {
        ctx->body = request.Body().value();
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDS, ctx->body.c_str());
        CURL_SETOPT_CHECK(curl, CURLOPT_POSTFIELDSIZE, ctx->body.size());
    }

    // Set headers
    auto const& base_headers = request.Properties().BaseHeaders();
    for (auto const& [key, value] : base_headers) {
        std::string header = key + kHeaderSeparator + value;
        const auto appendResult = curl_slist_append(headers, header.c_str());
        if (!appendResult) {
            if (headers) {
                curl_slist_free_all(headers);
            }
            ctx->callback(HttpResult(kErrorHeaderAppend));
            return;
        }
        headers = appendResult;
    }
    if (headers) {
        CURL_SETOPT_CHECK(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set timeouts with millisecond precision
    const long connect_timeout_ms = request.Properties().ConnectTimeout().count();
    const long response_timeout_ms = request.Properties().ResponseTimeout().count();

    CURL_SETOPT_CHECK(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms > 0 ? connect_timeout_ms : 30000L);
    CURL_SETOPT_CHECK(curl, CURLOPT_TIMEOUT_MS, response_timeout_ms > 0 ? response_timeout_ms : 60000L);

    // Set TLS options
    using VerifyMode = config::shared::built::TlsOptions::VerifyMode;
    if (tls_options.PeerVerifyMode() == VerifyMode::kVerifyNone) {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        // 1 or 2 seem to basically be the same, but the documentation says to
        // use 2, and that it would default to 2.
        // https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html
        CURL_SETOPT_CHECK(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Set custom CA file if provided
        if (tls_options.CustomCAFile().has_value()) {
            CURL_SETOPT_CHECK(curl, CURLOPT_CAINFO, tls_options.CustomCAFile()->c_str());
        }
    }

    // Set proxy if configured
    // When proxy URL is set, it takes precedence over environment variables.
    // Empty string explicitly disables proxy (overrides environment variables).
    auto const& proxy_url = request.Properties().Proxy().Url();
    if (proxy_url.has_value()) {
        CURL_SETOPT_CHECK(curl, CURLOPT_PROXY, proxy_url->c_str());
    }
    // If proxy URL is std::nullopt, CURL will use environment variables (default behavior)

    // Set callbacks
    CURL_SETOPT_CHECK(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    CURL_SETOPT_CHECK(curl, CURLOPT_WRITEDATA, &ctx->response_body);
    CURL_SETOPT_CHECK(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    CURL_SETOPT_CHECK(curl, CURLOPT_HEADERDATA, &ctx->response_headers);

    // Follow redirects
    CURL_SETOPT_CHECK(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURL_SETOPT_CHECK(curl, CURLOPT_MAXREDIRS, 20L);

    #undef CURL_SETOPT_CHECK

    // Add handle to multi manager for async processing
    // Headers will be freed automatically by CurlMultiManager
    multi_manager->add_handle(curl, headers, [ctx](CURL* easy, CURLcode result) {
        // This callback runs on the executor when the request completes

        // Check for errors
        if (result != CURLE_OK) {
            std::string error_message = kErrorCurlPrefix;
            error_message += curl_easy_strerror(result);
            ctx->callback(HttpResult(error_message));
            return;
        }

        // Get HTTP response code
        long response_code = 0;
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);

        // Invoke the user's callback with the result
        ctx->callback(HttpResult(
            static_cast<HttpResult::StatusCode>(response_code),
            std::move(ctx->response_body),
            std::move(ctx->response_headers)));
    });
}

}  // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING
