#include "launchdarkly/client_side/data_source_update_sink.hpp"

namespace launchdarkly::client_side {

bool operator==(ItemDescriptor const& lhs, ItemDescriptor const& rhs) {
    return lhs.version == rhs.version && lhs.flag == rhs.flag;
}

std::ostream& operator<<(std::ostream& out, ItemDescriptor const& descriptor) {
    out << "{";
    out << " version: " << descriptor.version;
    if (descriptor.flag.has_value()) {
        out << " flag: " << descriptor.flag.value();
    } else {
        out << " flag: <nullopt>";
    }
    return out;
}
ItemDescriptor::ItemDescriptor(uint64_t version) : version(version) {}

ItemDescriptor::ItemDescriptor(EvaluationResult flag)
    : version(flag.version()), flag(std::move(flag)) {}
}  // namespace launchdarkly::client_side
