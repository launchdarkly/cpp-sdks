#pragma once

#include <launchdarkly/detail/serialization/json_errors.hpp>

#include <boost/json/value.hpp>
#include <tl/expected.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace launchdarkly {

enum class IntentCode { kNone, kTransferFull, kTransferChanges, kUnknown };

struct ServerIntentPayload {
    std::string id;
    std::int64_t target;
    IntentCode intent_code;
    std::optional<std::string> reason;
};

struct ServerIntent {
    std::vector<ServerIntentPayload> payloads;
};

struct PutObject {
    std::int64_t version;
    std::string kind;
    std::string key;
    boost::json::value object;
};

struct DeleteObject {
    std::int64_t version;
    std::string kind;
    std::string key;
};

struct PayloadTransferred {
    std::string state;
    std::int64_t version;
};

struct Goodbye {
    std::optional<std::string> reason;
};

struct FDv2Error {
    std::optional<std::string> id;
    std::string reason;
};

tl::expected<std::optional<IntentCode>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<IntentCode>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<ServerIntentPayload>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<ServerIntentPayload>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::optional<ServerIntent>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<ServerIntent>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<PutObject>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<PutObject>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<DeleteObject>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<DeleteObject>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<PayloadTransferred>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<PayloadTransferred>, JsonError>> const&
        unused,
    boost::json::value const& json_value);

tl::expected<std::optional<Goodbye>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<Goodbye>, JsonError>> const& unused,
    boost::json::value const& json_value);

tl::expected<std::optional<FDv2Error>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<FDv2Error>, JsonError>> const& unused,
    boost::json::value const& json_value);

}  // namespace launchdarkly
