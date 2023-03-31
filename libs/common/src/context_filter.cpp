#include "context_filter.hpp"

#include <boost/json/src.hpp>

namespace launchdarkly {

ContextFilter::ContextFilter(
    bool all_attributes_private,
    AttributeReference::SetType const& global_private_attributes)
    : all_attributes_private_(all_attributes_private),
      global_private_attributes_(global_private_attributes) {}

ContextFilter::JsonValue ContextFilter::filter(Context const& context) {
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

void ContextFilter::emplace(ContextFilter::StackItem& item,
                            ContextFilter::JsonValue&& addition) {
    if (item.parent.is_object()) {
        item.parent.as_object().emplace(item.path.back(), std::move(addition));
    } else {
        item.parent.as_array().emplace_back(std::move(addition));
    }
}

bool ContextFilter::redact(std::vector<std::string>& redactions,
                           std::vector<std::string_view> path,
                           Attributes const& attributes) {
    if (all_attributes_private_) {
        redactions.push_back(
            AttributeReference::path_to_string_reference(path));
        return true;
    }
    auto global = std::find_if(
        global_private_attributes_.begin(), global_private_attributes_.end(),
        [&path](auto const& ref) { return ref.valid() && (ref == path); });
    if (global != global_private_attributes_.end()) {
        redactions.push_back(global->redaction_name());
        return true;
    }

    auto local = std::find_if(
        attributes.private_attributes().begin(),
        attributes.private_attributes().end(),
        [&path](auto const& ref) { return ref.valid() && (ref == path); });

    if (local != attributes.private_attributes().end()) {
        redactions.push_back(local->redaction_name());
        return true;
    }
    return false;
}

ContextFilter::JsonValue ContextFilter::filter_single_context(
    std::string_view kind,
    bool include_kind,
    Attributes const& attributes) {
    std::vector<StackItem> stack;
    JsonValue filtered = JsonObject();
    std::vector<std::string> redactions;

    filtered.as_object().insert_or_assign("key", attributes.key());
    if (include_kind) {
        filtered.as_object().insert_or_assign("kind", kind);
    }
    if (attributes.anonymous()) {
        filtered.as_object().insert_or_assign("anonymous",
                                              attributes.anonymous());
    }

    if (!attributes.name().empty() &&
        !redact(redactions, std::vector<std::string_view>{"name"},
                attributes)) {
        filtered.as_object().insert_or_assign("name", attributes.name());
    }

    for (auto const& pair : attributes.custom_attributes().as_object()) {
        stack.emplace_back(StackItem{
            pair.second, std::vector<std::string_view>{pair.first}, filtered});
    }

    while (!stack.empty()) {
        auto item = std::move(stack.back());
        stack.pop_back();

        // Check if the attribute needs redacted.
        if (!item.path.empty()) {
            if (redact(redactions, item.path, attributes)) {
                continue;
            }
        }

        if (item.value.is_object()) {
            JsonValue* nested;
            if (item.parent.is_object()) {
                item.parent.as_object().emplace(item.path.back(), JsonObject());
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
                item.parent.as_object().emplace(item.path.back(), JsonArray());
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

ContextFilter::JsonValue ContextFilter::filter_multi_context(
    Context const& context) {
    JsonValue filtered = JsonObject();
    filtered.as_object().emplace("kind", "multi");

    for (auto const& kind : context.kinds()) {
        filtered.as_object().emplace(
            kind, filter_single_context(kind, false,
                                        context.attributes(kind.data())));
    }

    return filtered;
}
}  // namespace launchdarkly
