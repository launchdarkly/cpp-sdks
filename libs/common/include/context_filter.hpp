#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <boost/json/src.hpp>
#include <boost/json.hpp>

#include "attribute_reference.hpp"
#include "context.hpp"

namespace launchdarkly {

class ContextFilter {
   public:
    using JsonValue = boost::json::value;
    using JsonObject = boost::json::object;
    using JsonArray = boost::json::array;

    ContextFilter(bool all_attributes_private,
                  AttributeReference::SetType const& global_private_attributes)
        : all_attributes_private_(all_attributes_private),
          global_private_attributes_(global_private_attributes) {}

    JsonValue filter(Context context) {
        if (!context.valid()) {
            // Should not happen.
            // TODO: Maybe assert.
            return {};
        }
        if (context.kinds().size() == 1) {
            auto kind = context.kinds()[0];
            return filter_single_context(kind, context.attributes(kind.data()));
        } else {
            return filter_multi_context(context);
        }
    }

   private:
    struct StackItem {
        Value const& value;
        std::vector<std::string_view> path;
        JsonValue& parent;
    };

    void emplace(StackItem& item, JsonValue&& addition) {
        if (item.parent.is_object()) {
            item.parent.as_object().emplace(item.path.back(),
                                            std::move(addition));
        } else {
            item.parent.as_array().emplace_back(std::move(addition));
        }
    }

    JsonValue filter_single_context(std::string_view kind,
                                    Attributes const& attributes) {
        std::vector<StackItem> stack;
        JsonValue filtered = JsonObject();

        filtered.as_object().insert_or_assign("key", attributes.key());
        filtered.as_object().insert_or_assign("kind", kind);
        if (attributes.anonymous()) {
            filtered.as_object().insert_or_assign("anonymous",
                                                  attributes.anonymous());
        }
        if (!attributes.name().empty()) {
            filtered.as_object().insert_or_assign("name", attributes.name());
        }

        for (auto const& pair : attributes.custom_attributes().as_object()) {
            stack.emplace_back(
                StackItem{pair.second,
                          std::vector<std::string_view>{pair.first}, filtered});
        }

        while (!stack.empty()) {
            auto item = std::move(stack.back());
            stack.pop_back();

            if (item.value.is_object()) {
                JsonValue* nested;
                if (item.parent.is_object()) {
                    item.parent.as_object().emplace(item.path.back(),
                                                    JsonObject());
                    nested = &item.parent.as_object().at(item.path.back());
                } else {
                    item.parent.as_array().emplace(0, JsonObject());
                    nested = &item.parent.as_array().at(0);
                }

                for (auto const& pair : item.value.as_object()) {
                    auto new_path = std::vector<std::string_view>(item.path);
                    new_path.push_back(pair.first);
                    stack.push_back(StackItem{
                        pair.second, new_path, *nested});
                }
            } else if (item.value.is_array()) {
                JsonValue* nested;
                if (item.parent.is_object()) {
                    item.parent.as_object().emplace(item.path.back(),
                                                    JsonArray());
                    nested = &item.parent.as_object().at(item.path.back());
                } else {
                    item.parent.as_array().emplace(0, JsonArray());
                    nested = &item.parent.as_array().at(0);
                }

                for (auto const& arr_item : item.value.as_array()) {
                    // Once inside an array the path doesn't matter anymore.
                    stack.push_back(StackItem{
                        arr_item, std::vector<std::string_view>{}, *nested});
                }
            } else {
                switch (item.value.type()) {
                    case Value::Type::kNull:
                        emplace(item, JsonValue());
                        break;
                    case Value::Type::kBool:
                        emplace(item, JsonValue(item.value.as_bool()));
                        break;
                    case Value::Type::kNumber:
                        emplace(item, JsonValue(item.value.as_double()));
                        break;
                    case Value::Type::kString:
                        emplace(item, JsonValue(item.value.as_string()));
                        break;
                    case Value::Type::kObject:
                        // Cannot happen.
                        break;
                    case Value::Type::kArray:
                        // Cannot happen.
                        break;
                }
            }
        }
        return filtered;
    }

    JsonValue filter_multi_context(Context const& context) {
        return JsonValue();
    }

    bool all_attributes_private_;
    AttributeReference::SetType const& global_private_attributes_;
};

}  // namespace launchdarkly