#include <algorithm>

#include <boost/core/ignore_unused.hpp>

#include "context_index.hpp"

#include "launchdarkly/serialization/value_mapping.hpp"

namespace launchdarkly::client_side::flag_manager {

ContextIndex::ContextIndex(ContextIndex::Index index)
    : index_(std::move(index)) {}

void ContextIndex::Notice(
    std::string id,
    std::chrono::time_point<std::chrono::system_clock> timestamp) {
    auto found = std::find_if(index_.begin(), index_.end(),
                              [&id](auto& entry) { return entry.id == id; });
    if (found != index_.end()) {
        found->timestamp = timestamp;
    } else {
        index_.push_back(IndexEntry{id, timestamp});
    }
}

std::vector<std::string> ContextIndex::Prune(std::size_t maxContexts) {
    if (index_.size() <= maxContexts) {
        return {};
    }

    std::sort(index_.begin(), index_.end(),
              [](IndexEntry const& a, IndexEntry const& b) {
                  return a.timestamp > b.timestamp;
              });

    Index removed = std::vector(
        index_.begin() +
            static_cast<Index::iterator::difference_type>(maxContexts),
        index_.end());

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
    boost::ignore_unused(unused);

    auto& top = json_value.emplace_object();
    auto arr = boost::json::array();

    for (auto& entry : index.Entries()) {
        auto obj = boost::json::object();
        obj.emplace("id", entry.id);
        obj.emplace("timestamp",
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        entry.timestamp.time_since_epoch())
                        .count());
        arr.emplace_back(std::move(obj));
    }
    top.emplace("index", arr);
}

ContextIndex tag_invoke(boost::json::value_to_tag<ContextIndex> const& unused,
                        boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    auto index = ContextIndex::Index();
    if (json_value.is_object()) {
        auto arr = json_value.as_object().find("index");
        if (arr != json_value.as_object().end() && arr->value().is_array()) {
            for (auto& item : arr->value().as_array()) {
                if (item.is_object()) {
                    auto& obj = item.as_object();
                    auto* id_iter = obj.find("id");
                    auto id = ValueAsOpt<std::string>(id_iter, obj.end());

                    auto* timestamp_iter = obj.find("timestamp");
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
    }
    return ContextIndex(index);
}

}  // namespace launchdarkly::client_side::flag_manager
