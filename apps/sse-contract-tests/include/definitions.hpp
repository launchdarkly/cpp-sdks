#pragma once

#include "nlohmann/json.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace nlohmann {

    template <typename T>
    struct adl_serializer<std::optional<T>> {
    static void to_json(json& j, const std::optional<T>& opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt; // this will call adl_serializer<T>::to_json which will
            // find the free function to_json in T's namespace!
        }
    }

    static void from_json(const json& j, std::optional<T>& opt) {
        if (j.is_null()) {
            opt = std::nullopt;
        } else {
            opt = j.get<T>(); // same as above, but with
            // adl_serializer<T>::from_json
        }
    }
};
} // namespace nlohmann


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
        body
);

struct event {
    std::string type;
    std::string data;
    std::string id;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(event, type, data, id);

struct event_message {
    std::string kind;
    event event;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(event_message, kind, event);

struct comment_message {
    std::string kind;
    std::string comment;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(comment_message, kind, comment);
