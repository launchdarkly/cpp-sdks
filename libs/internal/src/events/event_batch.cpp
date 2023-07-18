#include <launchdarkly/events/detail/event_batch.hpp>

#include <boost/json/serialize.hpp>

namespace launchdarkly::events::detail {
EventBatch::EventBatch(std::string url,
                       config::shared::built::HttpProperties http_props,
                       boost::json::value const& events)
    : num_events_(events.as_array().size()),
      request_(url,
               network::HttpMethod::kPost,
               http_props,
               boost::json::serialize(events)) {}

std::size_t EventBatch::Count() const {
    return num_events_;
}

network::HttpRequest const& EventBatch::Request() const {
    return request_;
}

std::string EventBatch::Target() const {
    return request_.Url();
}

}  // namespace launchdarkly::events::detail
