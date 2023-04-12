#pragma once

enum class JsonError {
    kSchemaFailure  // The JSON was valid, but didn't match our expected values.
};
