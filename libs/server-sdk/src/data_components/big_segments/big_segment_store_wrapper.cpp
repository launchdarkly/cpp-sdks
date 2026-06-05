#include "big_segment_store_wrapper.hpp"

#include <launchdarkly/async/timer.hpp>
#include <launchdarkly/encoding/base_64.hpp>
#include <launchdarkly/encoding/sha_256.hpp>
#include <launchdarkly/signals/boost_signal_connection.hpp>

#include <chrono>
#include <string>
#include <utility>
#include <variant>

namespace launchdarkly::server_side::data_components {

namespace {
// The store is keyed by base64(sha256(contextKey)), matching what the Relay
// Proxy writes; raw context keys are never sent to the store.
std::string HashContextKey(std::string const& context_key) {
    auto const digest = encoding::Sha256String(context_key);
    return encoding::Base64Encode(std::string(
        reinterpret_cast<char const*>(digest.data()), digest.size()));
}
}  // namespace

BigSegmentStoreWrapper::BigSegmentStoreWrapper(
    config::built::BigSegmentsConfig const& config,
    boost::asio::any_io_executor executor,
    Logger const& logger)
    : store_(config.store),
      stale_after_(config.stale_after),
      poll_interval_(config.status_poll_interval),
      logger_(logger),
      executor_(std::move(executor)),
      cache_(config.context_cache_size, config.context_cache_time) {}

BigSegmentStoreWrapper::~BigSegmentStoreWrapper() {
    poll_cancel_.Cancel();
}

void BigSegmentStoreWrapper::Start() {
    ScheduleNextPoll();
}

BigSegmentStoreWrapper::GetMembershipResult
BigSegmentStoreWrapper::GetMembership(std::string const& context_key) {
    auto membership = cache_.Get(context_key);
    if (!membership.has_value()) {
        auto const result = LoadMembership(context_key);
        if (!result.has_value()) {
            LD_LOG(logger_, LogLevel::kError)
                << "Big Segment store returned error: " << result.error();
            MarkStoreUnavailable();
            return {integrations::Membership::FromSegmentRefs({}, {}),
                    BigSegmentsStatus::kStoreError};
        }
        membership = *result;
    }

    auto const status = GetStatus().stale ? BigSegmentsStatus::kStale
                                          : BigSegmentsStatus::kHealthy;
    return {std::move(*membership), status};
}

BigSegmentStoreWrapper::StoreMembership BigSegmentStoreWrapper::LoadMembership(
    std::string const& context_key) {
    std::shared_ptr<InFlightQuery> query;
    bool is_leader = false;
    {
        std::lock_guard lock(load_mutex_);
        auto const it = in_flight_.find(context_key);
        if (it != in_flight_.end()) {
            query = it->second;
        } else {
            query = std::make_shared<InFlightQuery>();
            in_flight_.emplace(context_key, query);
            is_leader = true;
        }
    }

    if (!is_leader) {
        std::unique_lock lock(query->mutex);
        query->cv.wait(lock, [&query] { return query->result.has_value(); });
        return *query->result;
    }

    // Query the store outside any lock, then publish the result to waiters and
    // drop the in-flight entry.
    auto result = store_->GetMembership(HashContextKey(context_key));
    if (result.has_value()) {
        cache_.Set(context_key, *result);
    }
    {
        std::lock_guard lock(load_mutex_);
        in_flight_.erase(context_key);
    }
    {
        std::lock_guard lock(query->mutex);
        query->result = result;
    }
    query->cv.notify_all();
    return result;
}

void BigSegmentStoreWrapper::MarkStoreUnavailable() {
    BigSegmentStoreStatus new_status;
    bool changed;
    {
        std::lock_guard lock(status_mutex_);
        new_status.available = false;
        new_status.stale = last_status_.has_value() && last_status_->stale;
        changed = !last_status_.has_value() || *last_status_ != new_status;
        last_status_ = new_status;
    }
    if (changed) {
        status_signal_(new_status);
    }
}

void BigSegmentStoreWrapper::ScheduleNextPoll() {
    auto weak_self = weak_from_this();
    async::Delay(executor_, poll_interval_, poll_cancel_.GetToken())
        .Then(
            [weak_self](bool const& fired_normally) -> std::monostate {
                if (!fired_normally) {
                    // Cancelled (wrapper destroyed); stop polling.
                    return {};
                }
                if (auto self = weak_self.lock()) {
                    self->PollStoreAndUpdateStatus();
                    self->ScheduleNextPoll();
                }
                return {};
            },
            async::kInlineExecutor);
}

BigSegmentStoreStatus BigSegmentStoreWrapper::GetStatus() {
    {
        std::lock_guard lock(status_mutex_);
        if (last_status_.has_value()) {
            return *last_status_;
        }
    }
    // No poll has completed yet; do one now so the caller gets an accurate
    // staleness reading rather than a default. call_once coalesces concurrent
    // first-callers onto a single store query.
    std::call_once(first_poll_once_,
                   [this] { PollStoreAndUpdateStatus(); });
    std::lock_guard lock(status_mutex_);
    return *last_status_;
}

BigSegmentStoreStatus BigSegmentStoreWrapper::PollStoreAndUpdateStatus() {
    // Query the store outside the lock so a slow store doesn't block callers.
    auto const metadata = store_->GetMetadata();
    if (!metadata.has_value()) {
        LD_LOG(logger_, LogLevel::kError)
            << "Big Segment store status query returned error: "
            << metadata.error();
    }

    BigSegmentStoreStatus new_status;
    bool changed;
    {
        std::lock_guard lock(status_mutex_);
        if (metadata.has_value()) {
            new_status.available = true;
            new_status.stale =
                !metadata->has_value() || IsStale((*metadata)->last_up_to_date);
        } else {
            new_status.available = false;
            // Availability is bad; carry the last-known staleness forward.
            new_status.stale = last_status_.has_value() && last_status_->stale;
        }
        changed = !last_status_.has_value() || *last_status_ != new_status;
        last_status_ = new_status;
    }

    // Broadcast after releasing the lock so a listener can't deadlock against
    // it. Listeners are notified on the first poll and on any change.
    if (changed) {
        status_signal_(new_status);
    }
    return new_status;
}

bool BigSegmentStoreWrapper::IsStale(
    std::chrono::system_clock::time_point last_up_to_date) const {
    return (std::chrono::system_clock::now() - last_up_to_date) >= stale_after_;
}

std::unique_ptr<IConnection> BigSegmentStoreWrapper::OnStatusChange(
    std::function<void(BigSegmentStoreStatus)> handler) {
    return std::make_unique<internal::signals::SignalConnection>(
        status_signal_.connect(std::move(handler)));
}

}  // namespace launchdarkly::server_side::data_components
