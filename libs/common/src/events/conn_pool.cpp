#include "events/detail/conn_pool.hpp"
#include <iomanip>
#include <iostream>
#include "network/detail/asio_requester.hpp"

namespace launchdarkly::events::detail {
ConnPool::ConnPool(boost::asio::any_io_executor io, Logger& logger)
    : logger_(logger),
      requester_(std::move(io)),
      last_known_past_time_(),
      permanent_failure_(false) {}

void ConnPool::Deliver(RequestType request) {
    requester_.Request(
        std::move(request), [this](network::detail::HttpResult result) {
            if (result.IsError()) {
                LD_LOG(logger_, LogLevel::kError)
                    << "conn_pool: couldn't make request: "
                    << result.ErrorMessage().value_or("unknown error");
            }

            using namespace boost::beast;

            auto status = http::status(result.Status());

            switch (http::status_class(status)) {
                case http::status_class::successful: {
                    auto headers = result.Headers();
                    if (auto date_header = headers.find("Date");
                        date_header != headers.end()) {
                        // This is just... this must be improved.
                        std::tm t = {};
                        std::istringstream ss(date_header->second);
                        ss.imbue(std::locale("en_US.utf-8"));
                        ss >> std::get_time(&t, "%a, %d %b %Y %H:%M:%S GMT");
                        std::time_t tt = std::mktime(&t);

                        // Not thread safe.
                        last_known_past_time_ = Clock::from_time_t(tt);
                    }
                } break;
                case http::status_class::client_error:

                    if (status == http::status::bad_request ||
                        status == http::status::request_timeout ||
                        status == http::status::too_many_requests) {
                        // Not thread safe.
                        permanent_failure_ = false;
                    }
                    break;
                case http::status_class::unknown:
                    break;
                case http::status_class::informational:
                    break;
                case http::status_class::redirection:
                    break;
                case http::status_class::server_error:
                    break;
            }

            if (permanent_failure_) {
                // Do nothing.
            } else {
                // Send off another request after waiting 1 second.
            }
        });
}
std::optional<ConnPool::Clock::time_point> ConnPool::LastKnownPastTime() const {
    return last_known_past_time_;
}
bool ConnPool::PermanentFailure() const {
    return permanent_failure_;
}

}  // namespace launchdarkly::events::detail
