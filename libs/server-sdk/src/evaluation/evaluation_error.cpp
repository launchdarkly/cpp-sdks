#include "evaluation_error.hpp"
#include <boost/format.hpp>

namespace launchdarkly::server_side::evaluation {

Error::Error(char const* format, std::string arg)
    : format_{format}, arg_{std::move(arg)} {}

Error::Error(char const* format, std::int64_t arg)
    : Error(format, std::to_string(arg)) {}

Error::Error(char const* msg) : format_{msg}, arg_{std::nullopt} {}

Error Error::CyclicSegmentReference(std::string segment_key) {
    return {
        "segment rule referencing segment \"%1%\" caused a circular "
        "reference; this is probably a temporary condition due to an "
        "incomplete update",
        std::move(segment_key)};
}

Error Error::CyclicPrerequisiteReference(std::string prereq_key) {
    return {
        "prerequisite relationship to \"%1%\" caused a circular "
        "reference; this is probably a temporary condition due to an "
        "incomplete update",
        std::move(prereq_key)};
}

Error Error::RolloutMissingVariations() {
    return {"rollout or experiment with no variations"};
}

Error Error::InvalidAttributeReference(std::string ref) {
    return {"invalid attribute reference: \"%1%\"", std::move(ref)};
}

Error Error::NonexistentVariationIndex(std::int64_t index) {
    return {
        "rule, fallthrough, or target referenced a nonexistent variation index "
        "(%1%)",
        index};
}

std::ostream& operator<<(std::ostream& out, Error const& err) {
    if (err.arg_ == std::nullopt) {
        out << err.format_;
    } else {
        out << boost::format(err.format_) % *err.arg_;
    }
    return out;
}

bool operator==(Error const& lhs, Error const& rhs) {
    return lhs.format_ == rhs.format_ && lhs.arg_ == rhs.arg_;
}

}  // namespace launchdarkly::server_side::evaluation
