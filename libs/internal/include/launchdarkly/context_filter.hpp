#pragma once

#include <launchdarkly/attribute_reference.hpp>
#include <launchdarkly/context.hpp>

#include <boost/json.hpp>

#include <string>
#include <unordered_set>
#include <vector>

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
                  AttributeReference::SetType global_private_attributes);

    /**
     * Filter the given context and produce a JSON value.
     *
     * Only call this method for valid contexts.
     *
     * @param context The context to redact.
     * @return JSON suitable for an analytics event.
     */
    JsonValue Filter(Context const& context);

   private:
    /**
     * The filtering and JSON conversion algorithm is stack based
     * instead of recursive. Each node is visited, and if that node is a basic
     * type bool, number, string, then it is processed immediately, being
     * added to the output object. If the node is complex, then each of its
     * immediately children is added to the stack to be processed.
     */
    struct StackItem {
        Value const& value;
        std::vector<std::string_view> path;
        JsonValue& parent;
    };

    /**
     * Put an item into its parent. Either as a pair in a map,
     * or pushed onto an array.
     * @param item The stack item denoting placement information.
     * @param addition The item to add.
     */
    static void emplace(StackItem& item, JsonValue&& addition);

    /**
     * If the path needs to be redacted, then redact it and add it to the
     * redactions.
     * @param redactions The list of redacted items.
     * @param path The path to check.
     * @param attributes Attributes which may contain additional private
     * attributes.
     * @return True if the item was redacted.
     */
    bool Redact(std::vector<std::string>& redactions,
                std::vector<std::string_view> path,
                Attributes const& attributes);

    /**
     * Append a container to the parent.
     * @param item The stack item containing the parent.
     * @param value The container to append.
     * @return The appended container.
     */
    static JsonValue* AppendContainer(StackItem& item, JsonValue&& value);

    /**
     * Put a simple value into the parent specified by its stack item.
     * @param item The stack item with value information and the parent.
     */
    static void AppendSimpleType(StackItem& item);

    JsonValue FilterSingleContext(std::string_view kind,
                                    bool include_kind,
                                    Attributes const& attributes);

    JsonValue FilterMultiContext(Context const& context);

    bool all_attributes_private_;
    AttributeReference::SetType const global_private_attributes_;
};

}  // namespace launchdarkly
