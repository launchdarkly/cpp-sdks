#include <launchdarkly/server_side/big_segment_store_status.hpp>

namespace launchdarkly::server_side {

BigSegmentStoreStatus::BigSegmentStoreStatus(bool const available,
                                             bool const stale)
    : available_(available), stale_(stale) {}

bool BigSegmentStoreStatus::IsAvailable() const {
    return available_;
}

bool BigSegmentStoreStatus::IsStale() const {
    return stale_;
}

bool operator==(BigSegmentStoreStatus const& a,
                BigSegmentStoreStatus const& b) {
    return a.IsAvailable() == b.IsAvailable() && a.IsStale() == b.IsStale();
}

bool operator!=(BigSegmentStoreStatus const& a,
                BigSegmentStoreStatus const& b) {
    return !(a == b);
}

std::ostream& operator<<(std::ostream& out,
                         BigSegmentStoreStatus const& status) {
    out << "BigSegmentStoreStatus(available=" << std::boolalpha
        << status.IsAvailable() << ", stale=" << status.IsStale() << ")";
    return out;
}

}  // namespace launchdarkly::server_side
