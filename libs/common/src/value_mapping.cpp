#include "serialization/value_mapping.hpp"

namespace launchdarkly {

template <>
std::optional<uint64_t> ValueAsOpt(boost::json::object::const_iterator iterator,
                                   boost::json::object::const_iterator end) {
    if (iterator != end && iterator->value().is_number()) {
        return iterator->value().to_number<uint64_t>();
    }
    return std::nullopt;
}

template <>
std::optional<std::string> ValueAsOpt(boost::json::object::const_iterator iterator,
                                      boost::json::object::const_iterator end) {
    if (iterator != end && iterator->value().is_string()) {
        return std::string(iterator->value().as_string());
    }
    return std::nullopt;
}

template <>
bool ValueOrDefault(boost::json::object::const_iterator iterator,
                    boost::json::object::const_iterator end,
                    bool default_value) {
    if (iterator != end && iterator->value().is_bool()) {
        return iterator->value().as_bool();
    }
    return default_value;
}

template <>
std::string ValueOrDefault(boost::json::object::const_iterator iterator,
                           boost::json::object::const_iterator end,
                           std::string default_value) {
    if (iterator != end && iterator->value().is_string()) {
        return std::string(iterator->value().as_string());
    }
    return default_value;
}
template <>

uint64_t ValueOrDefault(boost::json::object::const_iterator iterator,
                        boost::json::object::const_iterator end,
                        uint64_t default_value) {
    if (iterator != end && iterator->value().is_number()) {
        return iterator->value().to_number<uint64_t>();
    }
    return default_value;
}

}  // namespace launchdarkly
