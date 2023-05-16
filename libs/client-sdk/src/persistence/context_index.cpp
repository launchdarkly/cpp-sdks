#include "context_index.hpp"

#include <algorithm>

namespace launchdarkly::persistence {

ContextIndex::ContextIndex(ContextIndex::Index index)
    : index_(std::move(index)) {}

void ContextIndex::Notice(std::string id) {
    auto now = std::chrono::system_clock::now();
    auto found = std::find_if(index_.begin(), index_.end(),
                              [&id](auto& entry) { return entry.id == id; });
    if (found != index_.end()) {
        found->timestamp = now;
    } else {
        index_.push_back(IndexEntry{id, now});
    }
}

std::vector<std::string> ContextIndex::Prune(int maxContexts) {
    if (index_.size() <= maxContexts) {
        return {};
    }

    std::sort(index_.begin(), index_.end(),
              [](IndexEntry const& a, IndexEntry const& b) {
                  return a.timestamp < b.timestamp;
              });

    Index removed = std::vector(index_.begin() + maxContexts, index_.end());

    index_.resize(maxContexts);

    std::vector<std::string> ids;

    std::transform(removed.begin(), removed.end(), std::back_inserter(ids),
                   [](auto& entry) { return entry.id; });

    return ids;
}

}  // namespace launchdarkly::persistence
