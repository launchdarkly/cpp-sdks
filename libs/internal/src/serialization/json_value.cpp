#include <boost/json.hpp>
#include <launchdarkly/detail/unreachable.hpp>
#include <launchdarkly/serialization/json_value.hpp>
#include <launchdarkly/serialization/value_mapping.hpp>

namespace launchdarkly {
// NOLINTBEGIN modernize-return-braced-init-list

// Braced initializer list is not the same for a single item as the
// constructors. Replacing them with braced init lists would result in all types
// being lists.

tl::expected<std::optional<Value>, JsonError> tag_invoke(
    boost::json::value_to_tag<
        tl::expected<std::optional<Value>, JsonError>> const& unused,
    boost::json::value const& json_value) {
    boost::ignore_unused(unused);

    // The name of the function needs to be tag_invoke for boost::json.

    // The conditions in these switches explicitly use the constructors, because
    // otherwise it is an init list, which is an array.
    switch (json_value.kind()) {
        case boost::json::kind::null:
            return Value();
        case boost::json::kind::bool_:
            return Value(json_value.as_bool());
        case boost::json::kind::int64:
            return Value(json_value.to_number<double>());
        case boost::json::kind::uint64:
            return Value(json_value.to_number<double>());
        case boost::json::kind::double_:
            return Value(json_value.as_double());
        case boost::json::kind::string:
            return Value(std::string(json_value.as_string()));
        case boost::json::kind::array: {
            auto vec = json_value.as_array();
            std::vector<Value> values;
            for (auto const& item : vec) {
                auto value =
                    boost::json::value_to<tl::expected<Value, JsonError>>(item);
                if (!value) {
                    return tl::make_unexpected(value.error());
                }
                values.emplace_back(std::move(*value));
            }
            return Value(values);
        }
        case boost::json::kind::object: {
            auto& map = json_value.as_object();
            std::map<std::string, Value> values;
            for (auto const& pair : map) {
                auto value =
                    boost::json::value_to<tl::expected<Value, JsonError>>(
                        pair.value());
                if (!value) {
                    return tl::make_unexpected(value.error());
                }
                values.emplace(pair.key().data(), std::move(*value));
            }
            return Value(std::move(values));
        }
    }
    // The above switch is exhaustive, so this can only happen if a new
    // type is added to boost::json::value.
    launchdarkly::detail::unreachable();
}

Value tag_invoke(boost::json::value_to_tag<Value> const&,
                 boost::json::value const& json_value) {
    auto val =
        boost::json::value_to<tl::expected<Value, JsonError>>(json_value);
    return val ? std::move(*val) : Value();
}

void tag_invoke(boost::json::value_from_tag const&,
                boost::json::value& json_value,
                Value const& ld_value) {
    switch (ld_value.Type()) {
        case Value::Type::kNull:
            json_value.emplace_null();
            break;
        case Value::Type::kBool:
            json_value.emplace_bool() = ld_value.AsBool();
            break;
        case Value::Type::kNumber:
            json_value.emplace_double() = ld_value.AsDouble();
            break;
        case Value::Type::kString:
            json_value.emplace_string() = ld_value.AsString();
            break;
        case Value::Type::kObject: {
            auto& obj = json_value.emplace_object();
            for (auto const& pair : ld_value.AsObject()) {
                obj.insert_or_assign(pair.first.c_str(),
                                     boost::json::value_from(pair.second));
            }
        } break;
        case Value::Type::kArray: {
            auto& arr = json_value.emplace_array();
            for (auto const& val : ld_value.AsArray()) {
                arr.push_back(boost::json::value_from(val));
            }
        } break;
    }
}

// NOLINTEND modernize-return-braced-init-list
}  // namespace launchdarkly
