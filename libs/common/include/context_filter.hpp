#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <boost/json.hpp>

#include "attribute_reference.hpp"
#include "context.hpp"

namespace launchdarkly {

/**
 * Class used by the SDK for filtering contexts to produce redacted JSON
 * for analytics events.
 */
class ContextFilter {
   public:
    using JsonValue = boost::json::value;
    using JsonObject = boost::json::object;
    using JsonArray = boost::json::array;

    ContextFilter(bool all_attributes_private,
                  AttributeReference::SetType const& global_private_attributes);

    /**
     * Filter the given context and produce a JSON value.
     * @param context The context to redact.
     * @return JSON suitable for an analytics event.
     */
    JsonValue filter(Context const& context);

   private:
    struct StackItem {
        Value const& value;
        std::vector<std::string_view> path;
        JsonValue& parent;
    };

    static void emplace(StackItem& item, JsonValue&& addition);

    bool redact(std::vector<std::string>& redactions,
                std::vector<std::string_view> path,
                Attributes const& attributes);

    JsonValue filter_single_context(std::string_view kind,
                                    bool include_kind,
                                    Attributes const& attributes);

    JsonValue filter_multi_context(Context const& context);

    bool all_attributes_private_;
    AttributeReference::SetType const& global_private_attributes_;
};

}  // namespace launchdarkly