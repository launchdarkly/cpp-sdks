#include "launchdarkly/serialization/json_context.hpp"
#include "launchdarkly/serialization/json_attributes.hpp"

namespace launchdarkly {
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Context const& ld_context) {
    if (ld_context.valid()) {
        if (ld_context.kinds().size() == 1) {
            auto kind = ld_context.kinds()[0].data();
            auto& obj = json_value.emplace_object() =
                std::move(boost::json::value_from(ld_context.attributes(kind))
                              .as_object());
            obj.emplace("kind", kind);
        } else {
            auto& obj = json_value.emplace_object();
            obj.emplace("kind", "multi");
            for (auto const& kind : ld_context.kinds()) {
                obj.emplace(kind.data(),
                            boost::json::value_from(
                                ld_context.attributes(kind.data())));
            }
        }
    }
}
}  // namespace launchdarkly
