
#include <type_traits>

#include "config/detail/sdks.hpp"

namespace launchdarkly::config::detail {

template <typename SDK, typename StreamingBuilder, typename PollingBuilder>
class DataSourceBuilder {
    DataSourceBuilder() : with_reasons_(false), use_report_(false) {}
    std::enable_if<std::is_same<SDK, ClientSDK>::value, bool>::type
    with_reasons(DataSourceBuilder value) {
        with_reasons_ = value;
        return *this;
    }

    std::enable_if<std::is_same<SDK, ClientSDK>::value, bool>::type use_report(
        DataSourceBuilder value) {
        use_report_ = value;
        return *this;
    }

   private:
    bool with_reasons_;
    bool use_report_;
};

}  // namespace launchdarkly::config::detail
