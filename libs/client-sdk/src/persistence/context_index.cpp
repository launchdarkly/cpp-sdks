#include <algorithm>

#include "context_index.hpp"

#include <launchdarkly/serialization/value_mapping.hpp>

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

ContextIndex::Index const& ContextIndex::Entries() const {
    return index_;
}

void tag_invoke(boost::json::value_from_tag const& unused,
                boost::json::value& json_value,
                ContextIndex const& index) {
    auto& arr = json_value.emplace_array();

    for (auto& entry : index.Entries()) {
        auto obj = boost::json::object();
        obj.emplace("id", entry.id);
        obj.emplace("timestamp",
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        entry.timestamp.time_since_epoch())
                        .count());
        arr.emplace_back(std::move(obj));
    }
}

ContextIndex tag_invoke(boost::json::value_to_tag<ContextIndex> const& unused,
                        boost::json::value const& json_value) {
    auto index = ContextIndex::Index();
    if (json_value.is_array()) {
        for (auto& item : json_value.as_array()) {
            if (item.is_object()) {
                auto& obj = item.as_object();
                auto* id_iter = obj.find("id");
                auto id = ValueAsOpt<std::string>(id_iter, obj.end());

                auto* timestamp_iter = obj.find("id");
                auto timestamp =
                    ValueAsOpt<uint64_t>(timestamp_iter, obj.end());

                if (id && timestamp) {
                    index.push_back(ContextIndex::IndexEntry{
                        *id, std::chrono::system_clock::time_point{
                                 std::chrono::milliseconds{*timestamp}}});
                }
            }
        }
    }
    return ContextIndex(index);
}

}  // namespace launchdarkly::persistence
