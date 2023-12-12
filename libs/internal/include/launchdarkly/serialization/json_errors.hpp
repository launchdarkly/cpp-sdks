#pragma once
#include <ostream>
namespace launchdarkly {
enum class JsonError {
    kSchemaFailure =
        0,  // Generic catchall for JSON not matching our expected values.
    kContextMissingKindField = 100,
    kContextInvalidKindField = 101,
    kContextMustBeObject = 102,
    kContextMissingKeyField = 103,
    kContextInvalidKeyField = 104,
    kContextInvalidNameField = 105,
    kContextInvalidAnonymousField = 106,
    kContextInvalidMetaField = 107,
    kContextInvalidSecondaryField = 108,
    kContextInvalidAttributeReference = 109,
    kContextInvalidPrivateAttributesField = 110,

    // The tombstone representation has a 'deleted' field, but it's false
    // instead of true as required.
    kTombstoneInvalidDeletedField = 200,
};

std::ostream& operator<<(std::ostream& os, JsonError const& err);

char const* ErrorToString(JsonError err);

}  // namespace launchdarkly
