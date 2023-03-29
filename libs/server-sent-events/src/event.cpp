#include <launchdarkly/sse/event.hpp>

namespace launchdarkly::sse {

Event::Event(std::string type, std::string data)
    : Event(std::move(type), std::move(data), std::nullopt) {}

Event::Event(std::string type, std::string data, std::string id)
    : Event(std::move(type),
            std::move(data),
            std::optional<std::string>{std::move(id)}) {}

Event::Event(std::string type, std::string data, std::optional<std::string> id)
    : type_(std::move(type)), data_(std::move(data)), id_(std::move(id)) {}

std::string const& Event::type() const {
    return type_;
}
std::string const& Event::data() const {
    return data_;
}
std::optional<std::string> const& Event::id() const {
    return id_;
}

std::string&& Event::take() && {
    return std::move(data_);
};

}  // namespace launchdarkly::sse
