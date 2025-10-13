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
    : ctx_(std::move(ctx)), tls_options_(tls_options) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void CurlRequester::Request(HttpRequest request, std::function<void(const HttpResult&)> cb) const {
    // Post the request to the executor to perform it asynchronously
    // Copy ctx_ and tls_options_ to avoid capturing 'this' and causing use-after-free
    // if the CurlRequester is destroyed while the operation is in flight.
    auto ctx = ctx_;
    auto tls_options = tls_options_;
    boost::asio::post(ctx, [ctx, tls_options, request = std::move(request), cb = std::move(cb)]() mutable {
        PerformRequestStatic(ctx, tls_options, std::move(request), std::move(cb));
    });
}

void CurlRequester::PerformRequestStatic(net::any_io_executor ctx, TlsOptions const& tls_options,
                                          const HttpRequest& request, std::function<void(const HttpResult&)> cb) {
    // Validate request
    if (!request.Valid()) {
        boost::asio::post(ctx, [cb = std::move(cb)]() {
            cb(HttpResult(kErrorMalformedRequest));
        });
        return;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        boost::asio::post(ctx, [cb = std::move(cb)]() {
            cb(HttpResult(kErrorCurlInit));
        });
        return;
    }

    // Use a unique_ptr to manage the cleanup of our curl instance.
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_guard(curl, curl_easy_cleanup);

    std::string response_body;
    HttpResult::HeadersType response_headers;

    // Store URL to keep it alive for the duration of the request
    std::string url = request.Url();

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // Set HTTP method
    if (request.Method() == HttpMethod::kPost) {
        // Basically CURLOPT_POST is a flag that indicates this is a post.
        // Passing 1 enables this flag.
        // This will also set a content type, but the headers for the request
        // should override that with the correct value.
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (request.Method() == HttpMethod::kPut) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, kHttpMethodPut);
    } else if (request.Method() == HttpMethod::kReport) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, kHttpMethodReport);
    } else if (request.Method() == HttpMethod::kGet) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    // Set request body if present
    if (request.Body().has_value()) {
        const std::string& body = request.Body().value();
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    }

    // Set headers
    struct curl_slist* headers = nullptr;
    auto const& base_headers = request.Properties().BaseHeaders();
    for (auto const& [key, value] : base_headers) {
        std::string header = key + kHeaderSeparator + value;
        // The first call to curl_slist_append will create the list.
        // Subsequent calls will return either the same pointer, or null.
        // In the case they return null, we need to clean up any previous result
        // and abort the operation.
        const auto appendResult = curl_slist_append(headers, header.c_str());
        if (!appendResult) {
            if (headers) {
                curl_slist_free_all(headers);
            }
            boost::asio::post(ctx, [cb = std::move(cb)]() {
                cb(HttpResult(kErrorHeaderAppend));
            });
            return;
        }
        headers = appendResult;
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // Set timeouts with millisecond precision
    const long connect_timeout_ms = request.Properties().ConnectTimeout().count();
    const long response_timeout_ms = request.Properties().ResponseTimeout().count();

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms > 0 ? connect_timeout_ms : 30000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, response_timeout_ms > 0 ? response_timeout_ms : 60000L);

    // Set TLS options
    using VerifyMode = config::shared::built::TlsOptions::VerifyMode;
    if (tls_options.PeerVerifyMode() == VerifyMode::kVerifyNone) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        // 1 or 2 seem to basically be the same, but the documentation says to
        // use 2, and that it would default to 2.
        // https://curl.se/libcurl/c/CURLOPT_SSL_VERIFYHOST.html
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Set custom CA file if provided
        if (tls_options.CustomCAFile().has_value()) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, tls_options.CustomCAFile()->c_str());
        }
    }

    // Set callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);

    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 20L);

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Cleanup headers
    if (headers) {
        curl_slist_free_all(headers);
    }

    // Check for errors
    if (res != CURLE_OK) {
        std::string error_message = kErrorCurlPrefix;
        error_message += curl_easy_strerror(res);
        boost::asio::post(ctx, [cb = std::move(cb), error_message = std::move(error_message)]() {
            cb(HttpResult(error_message));
        });
        return;
    }

    // Get HTTP response code
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    // Post the success result back to the executor
    boost::asio::post(ctx, [cb = std::move(cb), response_code,
                            response_body = std::move(response_body),
                            response_headers = std::move(response_headers)]() mutable {
        cb(HttpResult(static_cast<HttpResult::StatusCode>(response_code),
                      std::move(response_body),
                      std::move(response_headers)));
    });
}

}  // namespace launchdarkly::network

#endif  // LD_CURL_NETWORKING
