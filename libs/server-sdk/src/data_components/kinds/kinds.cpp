#include "kinds.hpp"
#include "launchdarkly/serialization/json_errors.hpp"

#include <launchdarkly/data_model/flag.hpp>
#include <launchdarkly/data_model/segment.hpp>

#include <boost/json/parse.hpp>

namespace launchdarkly::server_side::data_components {

template <typename TData>
static uint64_t GetVersion(std::string data) {
    boost::json::error_code error_code;
    auto parsed = boost::json::parse(data, error_code);

    if (error_code) {
        return 0;
    }
    auto res =
        boost::json::value_to<tl::expected<std::optional<TData>, JsonError>>(
            parsed);

    if (res.has_value() && res->has_value()) {
        return res->value().version;
    }
    return 0;
}

std::string const& SegmentKind::Namespace() const {
    return namespace_;
}

std::uint64_t SegmentKind::Version(std::string const& data) const {
    return GetVersion<data_model::Segment>(data);
}

std::string const& FlagKind::Namespace() const {
    return namespace_;
}

std::uint64_t FlagKind::Version(std::string const& data) const {
    return GetVersion<data_model::Flag>(data);
}

}  // namespace launchdarkly::server_side::data_components