#include <launchdarkly/server_side/integrations/data_reader/kinds.hpp>

#include <launchdarkly/serialization/json_flag.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_segment.hpp>

#include <boost/json.hpp>

namespace launchdarkly::server_side::integrations {

template <typename TData>
static uint64_t GetVersion(std::string const& data) {
    boost::json::error_code error_code;
    auto const parsed = boost::json::parse(data, error_code);

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

}  // namespace launchdarkly::server_side::integrations
