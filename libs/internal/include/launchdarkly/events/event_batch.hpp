#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <launchdarkly/network/http_requester.hpp>
#include <string>

namespace launchdarkly::events {

/**
 * EventBatch represents a batch of events being sent to LaunchDarkly as
 * an HTTP request.
 */
class EventBatch {
   public:
    /**
     * Constructs a new EventBatch.
     * @param url Target of the request.
     * @param http_props General HTTP properties for the request.
     * @param events Array of events to be serialized in the request.
     */
    EventBatch(std::string url,
               config::shared::built::HttpProperties http_props,
               boost::json::value const& events);

    /**
     * Returns the number of events in the batch.
     */
    [[nodiscard]] std::size_t Count() const;

    /**
     * Returns the built HTTP request.
     */
    [[nodiscard]] network::HttpRequest const& Request() const;

    /**
     * Returns the target of the request.
     */
    [[nodiscard]] std::string Target() const;

   private:
    std::size_t num_events_;
    network::HttpRequest request_;
};

}  // namespace launchdarkly::events
