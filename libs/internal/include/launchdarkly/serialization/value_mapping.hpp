#pragma once

#include <boost/core/ignore_unused.hpp>
#include <boost/json.hpp>

namespace launchdarkly {

template <typename Type>
std::optional<Type> ValueAsOpt(boost::json::object::const_iterator iterator,
                               boost::json::object::const_iterator end) {
    boost::ignore_unused(iterator);
    boost::ignore_unused(end);

    static_assert(sizeof(Type) == -1, "Must be specialized to use ValueAsOpt");
}

template <typename Type>
Type ValueOrDefault(boost::json::object::const_iterator iterator,
                    boost::json::object::const_iterator end,
                    Type default_value) {
    boost::ignore_unused(iterator);
    boost::ignore_unused(end);
    boost::ignore_unused(default_value);

    static_assert(sizeof(Type) == -1,
                  "Must be specialized to use ValueOrDefault");
}

template <typename OutType, typename InType>
std::optional<OutType> MapOpt(std::optional<InType> opt,
                              std::function<OutType(InType&)> const& mapper) {
    if (opt.has_value()) {
        return std::make_optional(mapper(opt.value()));
    }
    return std::nullopt;
}

template <>
std::optional<uint64_t> ValueAsOpt(boost::json::object::const_iterator iterator,
                                   boost::json::object::const_iterator end);

template <>
std::optional<std::string> ValueAsOpt(
    boost::json::object::const_iterator iterator,
    boost::json::object::const_iterator end);

template <>
bool ValueOrDefault(boost::json::object::const_iterator iterator,
                    boost::json::object::const_iterator end,
                    bool default_value);

template <>
uint64_t ValueOrDefault(boost::json::object::const_iterator iterator,
                        boost::json::object::const_iterator end,
                        uint64_t default_value);

template <>
std::string ValueOrDefault(boost::json::object::const_iterator iterator,
                           boost::json::object::const_iterator end,
                           std::string default_value);
}  // namespace launchdarkly
