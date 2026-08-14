#include "big_segment_store_status_provider.hpp"

#include <utility>

namespace launchdarkly::server_side::data_components {

namespace {

// Returned when no store is configured: nothing to disconnect.
class NoopConnection : public IConnection {
   public:
    void Disconnect() override {}
};

// Converts the internal status struct to the public status type. Both are named
// BigSegmentStoreStatus, in this namespace and in server_side respectively.
server_side::BigSegmentStoreStatus ToPublic(
    data_components::BigSegmentStoreStatus const& status) {
    return server_side::BigSegmentStoreStatus{status.available, status.stale};
}

}  // namespace

BigSegmentStoreStatusProvider::BigSegmentStoreStatusProvider(
    std::shared_ptr<BigSegmentStoreWrapper> wrapper)
    : wrapper_(std::move(wrapper)) {}

server_side::BigSegmentStoreStatus BigSegmentStoreStatusProvider::Status()
    const {
    if (!wrapper_) {
        return server_side::BigSegmentStoreStatus{/* available= */ false,
                                                  /* stale= */ false};
    }
    return ToPublic(wrapper_->GetStatus());
}

std::unique_ptr<IConnection>
BigSegmentStoreStatusProvider::OnBigSegmentStoreStatusChange(
    std::function<void(server_side::BigSegmentStoreStatus status)> handler) {
    if (!wrapper_) {
        return std::make_unique<NoopConnection>();
    }
    return wrapper_->OnStatusChange(
        [handler = std::move(handler)](
            data_components::BigSegmentStoreStatus status) {
            handler(ToPublic(status));
        });
}

}  // namespace launchdarkly::server_side::data_components
