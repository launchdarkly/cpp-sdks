#pragma once

#include <ostream>

namespace launchdarkly::common::data_sources {

/**
 * An enumeration describing the general type of an error.
 */
enum class DataSourceStatusErrorKind {
    /**
     * An unexpected error, such as an uncaught exception, further
     * described by the error message.
     */
    kUnknown = 0,

    /**
     * An I/O error such as a dropped connection.
     */
    kNetworkError = 1,

    /**
     * The LaunchDarkly service returned an HTTP response with an error
     * status, available in the status code.
     */
    kErrorResponse = 2,

    /**
     * The SDK received malformed data from the LaunchDarkly service.
     */
    kInvalidData = 3,

    /**
     * The data source itself is working, but when it tried to put an
     * update into the data store, the data store failed (so the SDK may
     * not have the latest data).
     */
    kStoreError = 4
};

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatusErrorKind const& kind);

}  // namespace launchdarkly::common::data_sources
