#include "serialization/json_errors.hpp"

namespace launchdarkly {

std::ostream& operator<<(std::ostream& os, JsonError const& err) {
    os << ErrorToString(err);
    return os;
}

char const* ErrorToString(JsonError err) {
    switch (err) {
        case JsonError::kSchemaFailure:
            return "unexpected JSON schema";
        case JsonError::kContextMissingKindField:
            return "context is missing 'kind' field";
        case JsonError::kContextInvalidKindField:
            return "context 'kind' field is invalid";
        case JsonError::kContextMustBeObject:
            return "context must be object";
        case JsonError::kContextMissingKeyField:
            return "context is missing 'key' field";
        case JsonError::kContextInvalidKeyField:
            return "context 'key' field is invalid";
        case JsonError::kContextInvalidNameField:
            return "context 'name' field is invalid";
        case JsonError::kContextInvalidAnonymousField:
            return "context 'anonymous' field is invalid";
        case JsonError::kContextInvalidMetaField:
            return "context '_meta' field is invalid";
        case JsonError::kContextInvalidSecondaryField:
            return "context 'secondary' field is invalid";
        case JsonError::kContextInvalidAttributeReference:
            return "context 'privateAttributes' field: invalid attribute "
                   "reference";
        case JsonError::kContextInvalidPrivateAttributesField:
            return "context 'privateAttributes' field is invalid";
    }
}

}  // namespace launchdarkly
