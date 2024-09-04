#include "payload_filter_validation.hpp"

#include <boost/regex.hpp>

namespace launchdarkly::server_side::data_systems::detail {
bool ValidateFilterKey(std::string const& filter_key) {
    if (filter_key.empty()) {
        return false;
    }
    try {
        return regex_search(filter_key,
                            boost::regex("^[a-zA-Z0-9][._\\-a-zA-Z0-9]*$"));
    } catch (boost::bad_expression) {
        // boost::bad_expression can be thrown by basic_regex when compiling a
        // regular expression.
        return false;
    } catch (std::runtime_error) {
        // std::runtime_error can be thrown when a call
        // to regex_search results in an "everlasting" search
        return false;
    }
}
}  // namespace launchdarkly::server_side::data_systems::detail
