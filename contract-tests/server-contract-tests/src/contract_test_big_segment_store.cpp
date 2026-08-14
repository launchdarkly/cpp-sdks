#include "contract_test_big_segment_store.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>

#include <chrono>
#include <optional>
#include <utility>
#include <vector>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace launchdarkly::server_side::integrations;

ContractTestBigSegmentStore::ContractTestBigSegmentStore(
    std::string callback_uri)
    : callback_uri_(std::move(callback_uri)) {}

tl::expected<nlohmann::json, std::string> ContractTestBigSegmentStore::Post(
    std::string const& path,
    nlohmann::json const& body) const noexcept {
    try {
        auto uri_result = boost::urls::parse_uri(callback_uri_);
        if (!uri_result) {
            return tl::make_unexpected("invalid callback URI: " +
                                       callback_uri_);
        }
        auto uri = *uri_result;
        std::string const host(uri.host());
        std::string const port =
            uri.has_port() ? std::string(uri.port()) : "80";
        std::string base(uri.path());
        // The callback URI carries a base path; the sub-path (/getMembership,
        // /getMetadata) is appended. Drop any trailing slash to avoid "//".
        if (!base.empty() && base.back() == '/') {
            base.pop_back();
        }
        std::string const target = base + path;

        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.connect(resolver.resolve(host, port));

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "cpp-server-sdk-contract-tests");
        req.set(http::field::content_type, "application/json");
        req.body() = body.dump();
        req.prepare_payload();
        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result_int() != 200) {
            return tl::make_unexpected(res.body());
        }
        return nlohmann::json::parse(res.body());
    } catch (std::exception const& e) {
        return tl::make_unexpected(e.what());
    }
}

ContractTestBigSegmentStore::GetMembershipResult
ContractTestBigSegmentStore::GetMembership(
    std::string const& context_hash) const noexcept {
    auto result = Post("/getMembership", {{"contextHash", context_hash}});
    if (!result) {
        return tl::make_unexpected(result.error());
    }
    try {
        std::vector<std::string> included;
        std::vector<std::string> excluded;
        auto const it = result->find("values");
        if (it != result->end() && it->is_object()) {
            for (auto const& [segment_ref, member] : it->items()) {
                (member.get<bool>() ? included : excluded)
                    .push_back(segment_ref);
            }
        }
        return Membership::FromSegmentRefs(included, excluded);
    } catch (std::exception const& e) {
        return tl::make_unexpected(e.what());
    }
}

ContractTestBigSegmentStore::GetMetadataResult
ContractTestBigSegmentStore::GetMetadata() const noexcept {
    auto result = Post("/getMetadata", nlohmann::json::object());
    if (!result) {
        return tl::make_unexpected(result.error());
    }
    try {
        auto const it = result->find("lastUpToDate");
        // Absent or zero means the store has never been synchronized.
        if (it == result->end() || it->is_null()) {
            return std::optional<StoreMetadata>{std::nullopt};
        }
        auto const millis = it->get<std::uint64_t>();
        if (millis == 0) {
            return std::optional<StoreMetadata>{std::nullopt};
        }
        return std::optional<StoreMetadata>{
            StoreMetadata{std::chrono::system_clock::time_point{
                std::chrono::milliseconds{millis}}}};
    } catch (std::exception const& e) {
        return tl::make_unexpected(e.what());
    }
}
