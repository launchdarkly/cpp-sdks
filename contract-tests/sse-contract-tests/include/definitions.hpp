#pragma once

#include <launchdarkly/sse/event.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, std::optional<T> const& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;  // this will call adl_serializer<T>::to_json which will
            // find the free function to_json in T's namespace!
        }
    }

    static void from_json(json const& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>();  // same as above, but with
            // adl_serializer<T>::from_json
        }
    }
};
}  // namespace nlohmann

// Represents the initial JSON configuration sent by the test harness.
struct ConfigParams {
    std::string streamUrl;
    std::string callbackUrl;
    std::string tag;
    std::optional<uint32_t> initialDelayMs;
    std::optional<uint32_t> readTimeoutMs;
    std::optional<std::string> lastEventId;
    std::optional<std::unordered_map<std::string, std::string>> headers;
    std::optional<std::string> method;
    std::optional<std::string> body;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ConfigParams,
                                                streamUrl,
                                                callbackUrl,
                                                tag,
                                                initialDelayMs,
                                                readTimeoutMs,
                                                lastEventId,
                                                headers,
                                                method,
                                                body);

// Represents an event payload that this service posts back
// to the test harness. The events are originally received by this server
// via the SSE stream; they are posted back so the test harness can verify
// that we parsed and dispatched them successfully.
struct Event {
    std::string type;
    std::string id;
    std::string data;
    Event() = default;
    explicit Event(launchdarkly::sse::Event event)
        : type(event.type()),
          id(event.id().value_or("")),
          data(std::move(event).take()) {}
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Event, type, data, id);

struct EventMessage {
    std::string kind;
    Event event;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(EventMessage, kind, event);

struct CommentMessage {
    std::string kind;
    std::string comment;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(CommentMessage, kind, comment);

struct ErrorMessage {
    std::string kind;
    std::string comment;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ErrorMessage, kind, comment);
