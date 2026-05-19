#include <launchdarkly/server_side/integrations/big_segments/big_segment_store_types.hpp>

#include <utility>

namespace launchdarkly::server_side::integrations {

Membership::Membership(std::unordered_map<std::string, bool> entries)
    : entries_(std::move(entries)) {}

Membership Membership::FromSegmentRefs(
    std::vector<std::string> const& included_segment_refs,
    std::vector<std::string> const& excluded_segment_refs) {
    std::unordered_map<std::string, bool> entries;
    // Excluded first so that any overlap is overwritten by the included pass;
    // inclusion wins per the Big Segments spec.
    for (auto const& ref : excluded_segment_refs) {
        entries[ref] = false;
    }
    for (auto const& ref : included_segment_refs) {
        entries[ref] = true;
    }
    return Membership(std::move(entries));
}

std::optional<bool> Membership::CheckMembership(
    std::string const& segment_ref) const {
    auto const it = entries_.find(segment_ref);
    if (it == entries_.end()) {
        return std::nullopt;
    }
    return it->second;
}

}  // namespace launchdarkly::server_side::integrations
