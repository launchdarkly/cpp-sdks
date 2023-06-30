#include <launchdarkly/context_builder.hpp>
#include <launchdarkly/serialization/json_attributes.hpp>
#include <launchdarkly/serialization/json_context.hpp>
#include <launchdarkly/serialization/json_primitives.hpp>
#include <launchdarkly/serialization/json_value.hpp>

#include <boost/json.hpp>

#include <optional>

namespace launchdarkly {
void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Context const& ld_context) {
    if (ld_context.Valid()) {
        if (ld_context.Kinds().size() == 1) {
            auto kind = ld_context.Kinds()[0].data();
            auto& obj = json_value.emplace_object() =
                std::move(boost::json::value_from(ld_context.Attributes(kind))
                              .as_object());
            obj.emplace("kind", kind);
        } else {
            auto& obj = json_value.emplace_object();
            obj.emplace("kind", "multi");
            for (auto const& kind : ld_context.Kinds()) {
                obj.emplace(kind.data(),
                            boost::json::value_from(
                                ld_context.Attributes(kind.data())));
            }
        }
    }
}

std::optional<JsonError> ParseSingle(ContextBuilder& builder,
                                     boost::json::object const& context,
                                     boost::json::string const& kind) {
    auto* key_iter = context.find("key");
    if (key_iter == context.end()) {
        return JsonError::kContextMissingKeyField;
    }
    if (!key_iter->value().is_string()) {
        return JsonError::kContextInvalidKeyField;
    }
    std::string key = boost::json::value_to<std::string>(key_iter->value());

    auto& attrs = builder.Kind(std::string(kind), key);

    auto* name_iter = context.find("name");
    if (name_iter != context.end() && !name_iter->value().is_null()) {
        if (name_iter->value().is_string()) {
            attrs.Name(boost::json::value_to<std::string>(name_iter->value()));
        } else {
            return JsonError::kContextInvalidNameField;
        }
    }

    auto* anon_iter = context.find("anonymous");
    if (anon_iter != context.end() && !anon_iter->value().is_null()) {
        if (anon_iter->value().is_bool()) {
            attrs.Anonymous(boost::json::value_to<bool>(anon_iter->value()));
        } else {
            return JsonError::kContextInvalidAnonymousField;
        }
    }

    auto* meta_iter = context.find("_meta");
    if (meta_iter != context.end() && !meta_iter->value().is_null()) {
        if (!meta_iter->value().is_object()) {
            return JsonError::kContextInvalidMetaField;
        }

        auto const& meta = meta_iter->value().as_object();

        auto secondary_iter = meta.find("secondary");
        if (secondary_iter != meta.end() &&
            !secondary_iter->value().is_null()) {
            if (secondary_iter->value().is_string()) {
                // TODO: how to set secondary, without exposing in public
                // interface of builder
            } else {
                return JsonError::kContextInvalidSecondaryField;
            }
        }

        auto private_attrs_iter = meta.find("privateAttributes");
        if (private_attrs_iter != meta.end() &&
            !private_attrs_iter->value().is_null()) {
            if (!private_attrs_iter->value().is_array()) {
                return JsonError::kContextInvalidPrivateAttributesField;
            }
            auto const& private_attrs = private_attrs_iter->value().as_array();
            for (auto const& attr : private_attrs) {
                if (!attr.is_string()) {
                    return JsonError::kContextInvalidAttributeReference;
                }
                attrs.AddPrivateAttribute(
                    boost::json::value_to<std::string>(attr));
            }
        }
    }

    for (auto attr = context.begin(); attr != context.end(); attr++) {
        if (attr == key_iter || attr == name_iter || attr == anon_iter ||
            attr == meta_iter || attr->value().is_null()) {
            continue;
        }
        auto maybe_unmarshalled_attr =
            boost::json::value_to<tl::expected<Value, JsonError>>(
                attr->value());
        if (maybe_unmarshalled_attr) {
            attrs.Set(attr->key(), maybe_unmarshalled_attr.value());
        }
    }

    return std::nullopt;
}

tl::expected<Context, JsonError> tag_invoke(
    boost::json::value_to_tag<tl::expected<Context, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    if (!json_value.is_object()) {
        return tl::make_unexpected(JsonError::kContextMustBeObject);
    }

    auto const& context = json_value.as_object();

    auto* kind_iter = context.find("kind");

    if (kind_iter == context.end()) {
        return tl::make_unexpected(JsonError::kContextMissingKindField);
    }

    if (!kind_iter->value().is_string()) {
        return tl::make_unexpected(JsonError::kContextInvalidKindField);
    }

    auto const& kind = kind_iter->value().as_string();

    auto builder = ContextBuilder();

    if (kind == "multi") {
        for (auto single_kind = context.begin(); single_kind != context.end();
             single_kind++) {
            if (single_kind == kind_iter) {
                continue;
            }
            if (!single_kind->value().is_object()) {
                return tl::make_unexpected(JsonError::kContextMustBeObject);
            }
            if (auto err =
                    ParseSingle(builder, single_kind->value().as_object(),
                                single_kind->key())) {
                return tl::make_unexpected(*err);
            }
        }
    } else if (auto err = ParseSingle(builder, context, kind)) {
        return tl::make_unexpected(*err);
    }

    return builder.Build();
}
}  // namespace launchdarkly
