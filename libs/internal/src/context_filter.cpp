#include <launchdarkly/context_filter.hpp>

#define INCLUDE_KIND true
#define EXCLUDE_KIND false

namespace launchdarkly {

ContextFilter::ContextFilter(
    bool all_attributes_private,
    AttributeReference::SetType global_private_attributes)
    : all_attributes_private_(all_attributes_private),
      global_private_attributes_(std::move(global_private_attributes)) {}

ContextFilter::JsonValue ContextFilter::Filter(Context const& context) {
    // Context should be validated before calling this method.
    assert(context.Valid());
    if (context.Kinds().size() == 1) {
        std::string const& kind = context.Kinds()[0];
        return FilterSingleContext(kind, INCLUDE_KIND,
                                   context.Attributes(kind));
    }
    return FilterMultiContext(context);
}

void ContextFilter::Emplace(ContextFilter::StackItem& item,
                            ContextFilter::JsonValue&& addition) {
    if (item.parent.is_object()) {
        item.parent.as_object().emplace(item.path.back(), std::move(addition));
    } else {
        item.parent.as_array().emplace_back(std::move(addition));
    }
}

bool ContextFilter::Redact(std::vector<std::string>& redactions,
                           std::vector<std::string_view> path,
                           Attributes const& attributes) {
    if (all_attributes_private_) {
        redactions.push_back(AttributeReference::PathToStringReference(path));
        return true;
    }
    auto global = std::find_if(
        global_private_attributes_.begin(), global_private_attributes_.end(),
        [&path](auto const& ref) { return ref.Valid() && (ref == path); });
    if (global != global_private_attributes_.end()) {
        redactions.push_back(global->RedactionName());
        return true;
    }

    auto local = std::find_if(
        attributes.PrivateAttributes().begin(),
        attributes.PrivateAttributes().end(),
        [&path](auto const& ref) { return ref.Valid() && (ref == path); });

    if (local != attributes.PrivateAttributes().end()) {
        redactions.push_back(local->RedactionName());
        return true;
    }
    return false;
}

ContextFilter::JsonValue ContextFilter::FilterSingleContext(
    std::string_view kind,
    bool include_kind,
    Attributes const& attributes) {
    std::vector<StackItem> stack;
    JsonValue filtered = JsonObject();
    std::vector<std::string> redactions;

    filtered.as_object().emplace("key", attributes.Key());
    if (include_kind) {
        filtered.as_object().emplace("kind", kind);
    }
    if (attributes.Anonymous()) {
        filtered.as_object().emplace("anonymous", attributes.Anonymous());
    }

    if (!attributes.Name().empty() &&
        !Redact(redactions, std::vector<std::string_view>{"name"},
                attributes)) {
        filtered.as_object().insert_or_assign("name", attributes.Name());
    }

    for (auto const& pair : attributes.CustomAttributes().AsObject()) {
        stack.emplace_back(StackItem{
            pair.second, std::vector<std::string_view>{pair.first}, filtered});
    }

    while (!stack.empty()) {
        auto item = std::move(stack.back());
        stack.pop_back();

        // Check if the attribute needs to be redacted.
        if (!item.path.empty() && Redact(redactions, item.path, attributes)) {
            continue;
        }

        if (item.value.IsObject()) {
            JsonValue* nested = AppendContainer(item, JsonObject());

            for (auto const& pair : item.value.AsObject()) {
                auto new_path = std::vector<std::string_view>(item.path);
                new_path.push_back(pair.first);
                stack.push_back(StackItem{pair.second, new_path, *nested});
            }
        } else if (item.value.IsArray()) {
            JsonValue* nested = AppendContainer(item, JsonArray());

            // Array contents are added in reverse, this is a recursive
            // algorithm so they will get reversed again when the stack
            // is processed.
            auto rev_until = std::reverse_iterator<Value::Array::Iterator>(
                item.value.AsArray().begin());
            auto rev_from = std::reverse_iterator<Value::Array::Iterator>(
                item.value.AsArray().end());
            while (rev_from != rev_until) {
                // Once inside an array the path doesn't matter anymore.
                // An item in an array cannot be marked private.
                stack.push_back(StackItem{
                    *rev_from, std::vector<std::string_view>{}, *nested});
                rev_from++;
            }
        } else {
            AppendSimpleType(item);
        }
    }

    // There were redactions, so we need to add _meta.
    if (!redactions.empty()) {
        auto obj = JsonObject();
        auto arr = JsonArray();
        for (auto const& redaction : redactions) {
            arr.push_back(JsonValue(redaction));
        }
        obj.emplace("redactedAttributes", std::move(arr));
        filtered.as_object().emplace("_meta", std::move(obj));
    }
    return filtered;
}

void ContextFilter::AppendSimpleType(ContextFilter::StackItem& item) {
    switch (item.value.Type()) {
        case Value::Type::kNull:
            Emplace(item, JsonValue());
            break;
        case Value::Type::kBool:
            Emplace(item, item.value.AsBool());
            break;
        case Value::Type::kNumber:
            Emplace(item, item.value.AsDouble());
            break;
        case Value::Type::kString:
            Emplace(item, item.value.AsString().c_str());
            break;
        case Value::Type::kObject:
        case Value::Type::kArray:
            // Will only happen if the code is extended incorrectly.
            assert(!"Arrays and objects must be handled before simple types.");
    }
}

ContextFilter::JsonValue* ContextFilter::AppendContainer(
    ContextFilter::StackItem& item,
    JsonValue&& value) {
    if (item.parent.is_object()) {
        item.parent.as_object().emplace(item.path.back(), std::move(value));
        return &item.parent.as_object().at(item.path.back());
    }
    item.parent.as_array().emplace_back(std::move(value));
    return &item.parent.as_array().back();
}

ContextFilter::JsonValue ContextFilter::FilterMultiContext(
    Context const& context) {
    JsonValue filtered = JsonObject();
    filtered.as_object().emplace("kind", "multi");

    for (std::string const& kind : context.Kinds()) {
        filtered.as_object().emplace(
            kind,
            FilterSingleContext(kind, EXCLUDE_KIND, context.Attributes(kind)));
    }

    return filtered;
}
}  // namespace launchdarkly
