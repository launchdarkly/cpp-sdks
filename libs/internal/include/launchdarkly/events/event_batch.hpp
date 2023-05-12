#pragma once

#include <boost/json/value.hpp>
#include <launchdarkly/config/shared/built/http_properties.hpp>
#include <string>
#include "launchdarkly/network/http_requester.hpp"

namespace launchdarkly::events::detail {

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
               config::detail::built::HttpProperties http_props,
               boost::json::value const& events);

    /**
     * Returns the number of events in the batch.
     */
    [[nodiscard]] std::size_t Count() const;

    /**
     * Returns the built HTTP request.
     */
    [[nodiscard]] network::detail::HttpRequest const& Request() const;

    /**
     * Returns the target of the request.
     */
    [[nodiscard]] std::string Target() const;

   private:
    std::size_t num_events_;
    network::detail::HttpRequest request_;
};

}  // namespace launchdarkly::events::detail
