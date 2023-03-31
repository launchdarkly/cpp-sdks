#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include <boost/json.hpp>
#include <boost/json/src.hpp>

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

    JsonValue filter(Context const& context) {
        if (!context.valid()) {
            // Should not happen.
            // TODO: Maybe assert.
            return {};
        }
        if (context.kinds().size() == 1) {
            auto kind = context.kinds()[0];
            return filter_single_context(kind, true,
                                         context.attributes(kind.data()));
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
                                    bool include_kind,
                                    Attributes const& attributes) {
        std::vector<StackItem> stack;
        JsonValue filtered = JsonObject();

        filtered.as_object().insert_or_assign("key", attributes.key());
        if (include_kind) {
            filtered.as_object().insert_or_assign("kind", kind);
        }
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

        std::vector<std::string_view> redactions;

        while (!stack.empty()) {
            auto item = std::move(stack.back());
            stack.pop_back();

            // Check if the attribute needs redacted.
            if (!item.path.empty()) {
                auto global = std::find_if(
                    global_private_attributes_.begin(),
                    global_private_attributes_.end(), [&item](auto const& ref) {
                        return ref.valid() && (ref == item.path);
                    });
                if (global != global_private_attributes_.end()) {
                    redactions.push_back(global->redaction_name());
                    continue;
                }

                auto local =
                    std::find_if(attributes.private_attributes().begin(),
                                 attributes.private_attributes().end(),
                                 [&item](auto const& ref) {
                                     return ref.valid() && (ref == item.path);
                                 });

                if (local != attributes.private_attributes().end()) {
                    redactions.push_back(local->redaction_name());
                    continue;
                }
            }

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
                    stack.push_back(StackItem{pair.second, new_path, *nested});
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

                // Array contents are added in reverse, this is a recursive
                // algorithm so they will get reversed again when the stack
                // is processed.
                auto rev_until = std::reverse_iterator<Value::Array::Iterator>(
                    item.value.as_array().begin());
                auto rev_from = std::reverse_iterator<Value::Array::Iterator>(
                    item.value.as_array().end());
                while (rev_from != rev_until) {
                    // Once inside an array the path doesn't matter anymore.
                    // An item in an array cannot be marked private.
                    stack.push_back(StackItem{
                        *rev_from, std::vector<std::string_view>{}, *nested});
                    *rev_from++;
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

        if (!redactions.empty()) {
            auto obj = JsonObject();
            auto arr = JsonArray();
            for (auto redaction : redactions) {
                arr.push_back(JsonValue(redaction));
            }
            obj.emplace("redactedAttributes", std::move(arr));
            filtered.as_object().emplace("_meta", std::move(obj));
        }
        return filtered;
    }

    JsonValue filter_multi_context(Context const& context) {
        JsonValue filtered = JsonObject();
        filtered.as_object().emplace("kind", "multi");

        for (auto const& kind : context.kinds()) {
            filtered.as_object().emplace(
                kind, filter_single_context(kind, false,
                                            context.attributes(kind.data())));
        }

        return filtered;
    }

    bool all_attributes_private_;
    AttributeReference::SetType const& global_private_attributes_;
};

}  // namespace launchdarkly