#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <unordered_map>

namespace nlohmann {

    template <class T>
    void to_json(nlohmann::json& j, const std::optional<T>& v)
    {
        if (v.has_value())
            j = *v;
        else
            j = nullptr;
    }

    template <class T>
    void from_json(const nlohmann::json& j, std::optional<T>& v)
    {
        if (j.is_null())
            v = std::nullopt;
        else
            v = j.get<T>();
    }
} // namespace nlohmann


struct config_params {
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(config_params,
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
