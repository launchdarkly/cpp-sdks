#pragma once

#include <boost/json.hpp>
#include <launchdarkly/serialization/json_errors.hpp>
#include <memory>
#include <optional>
#include <ostream>
#include <tl/expected.hpp>
#include <unordered_map>

namespace launchdarkly {
/**
 * An item descriptor is an abstraction that allows for Flag data to be
 * handled using the same type in both a put or a patch.
 */
template <typename T>
struct ItemDescriptor {
    /**
     * The version number of this data, provided by the SDK.
     */
    uint64_t version;

    /**
     * The data item, or nullopt if this is a deleted item placeholder.
     */
    std::optional<T> flag;

    explicit ItemDescriptor(uint64_t version);

    explicit ItemDescriptor(T flag);

    ItemDescriptor(ItemDescriptor const& item) = default;
    ItemDescriptor(ItemDescriptor&& item) = default;
    ItemDescriptor& operator=(ItemDescriptor const&) = default;
    ItemDescriptor& operator=(ItemDescriptor&&) = default;
    ~ItemDescriptor() = default;

    friend std::ostream& operator<<(std::ostream& out,
                                    ItemDescriptor const& descriptor);
};

template <typename T>
bool operator==(ItemDescriptor<T> const& lhs, ItemDescriptor<T> const& rhs) {
    return lhs.version == rhs.version && lhs.flag == rhs.flag;
}

template <typename T>
std::ostream& operator<<(std::ostream& out,
                         ItemDescriptor<T> const& descriptor) {
    out << "{";
    out << " version: " << descriptor.version;
    if (descriptor.flag.has_value()) {
        out << " flag: " << descriptor.flag.value();
    } else {
        out << " flag: <nullopt>";
    }
    return out;
}

template <typename T>
ItemDescriptor<T>::ItemDescriptor(uint64_t version) : version(version) {}

template <typename T>
ItemDescriptor<T>::ItemDescriptor(T flag)
    : version(flag.Version()), flag(std::move(flag)) {}

template <typename T>
tl::expected<std::unordered_map<std::string, ItemDescriptor<T>>, JsonError>
tag_invoke(boost::json::value_to_tag<
               tl::expected<std::unordered_map<std::string, ItemDescriptor<T>>,
                            JsonError>> const& unused,
           boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::unexpected(JsonError::kSchemaFailure);
    }
    auto const& obj = json_value.as_object();
    std::unordered_map<std::string, ItemDescriptor<T>> descriptors;
    for (auto const& pair : obj) {
        auto eval_result =
            boost::json::value_to<tl::expected<T, JsonError>>(pair.value());
        if (!eval_result.has_value()) {
            return tl::unexpected(JsonError::kSchemaFailure);
        }
        descriptors.emplace(pair.key(),
                            ItemDescriptor<T>(std::move(eval_result.value())));
    }
    return descriptors;
}

template <typename T>
void tag_invoke(
    boost::json::value_from_tag const& unused,
    boost::json::value& json_value,
    std::unordered_map<std::string, std::shared_ptr<ItemDescriptor<T>>> const&
        all_flags) {
    boost::ignore_unused(unused);

    auto& obj = json_value.emplace_object();
    for (auto descriptor : all_flags) {
        // Only serialize non-deleted flags.
        if (descriptor.second->flag) {
            auto eval_result_json =
                boost::json::value_from(*descriptor.second->flag);
            obj.emplace(descriptor.first, eval_result_json);
        }
    }
}

}  // namespace launchdarkly
