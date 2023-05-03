#include "events/detail/event_batch.hpp"
#include <boost/json/serialize.hpp>

namespace launchdarkly::events::detail {
EventBatch::EventBatch(std::string url,
                       config::detail::built::HttpProperties http_props,
                       boost::json::value const& events)
    : num_events_(events.as_array().size()),
      request_(url,
               network::detail::HttpMethod::kPost,
               http_props,
               boost::json::serialize(events)) {}

std::size_t EventBatch::Count() const {
    return num_events_;
}

network::detail::HttpRequest const& EventBatch::Request() const {
    return request_;
}

std::string EventBatch::Target() const {
    return (request_.Https() ? "https://" : "http://") + request_.Host() + ":" +
           request_.Port() + request_.Path();
}

}  // namespace launchdarkly::events::detail
