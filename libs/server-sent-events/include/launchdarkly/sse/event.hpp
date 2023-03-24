#pragma once

#include <optional>
#include <string>

namespace launchdarkly::sse {

class Event {
   public:
    Event(std::string type, std::string data);
    Event(std::string type, std::string data, std::string id);
    Event(std::string type, std::string data, std::optional<std::string> id);
    std::string const& type() const;
    std::string const& data() const;
    std::optional<std::string> const& id() const;
    std::string&& take();

   private:
    std::string type_;
    std::string data_;
    std::optional<std::string> id_;
};

}  // namespace launchdarkly::sse
