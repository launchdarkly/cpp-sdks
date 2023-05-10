#pragma once

enum class JsonError {
    kSchemaFailure =
        0,  // Generic catchall for JSON not matching our expected values.
    kContextMissingKindField = 100,
    kContextInvalidKindField = 101,
    kContextMustBeObject = 102,
    kContextMissingKeyField = 103,
    kContextInvalidKeyField = 104,
    kContextInvalidNameField = 105,
    kContextInvalidAnonymousField = 106
};
