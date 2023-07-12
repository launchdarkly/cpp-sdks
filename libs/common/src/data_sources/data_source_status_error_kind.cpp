#include <iomanip>
#include <ostream>

#include <launchdarkly/data_sources/data_source_status_error_kind.hpp>

namespace launchdarkly::common::data_sources {

std::ostream& operator<<(std::ostream& out,
                         DataSourceStatusErrorKind const& kind) {
    switch (kind) {
        case DataSourceStatusErrorKind::kUnknown:
            out << "UNKNOWN";
            break;
        case DataSourceStatusErrorKind::kNetworkError:
            out << "NETWORK_ERROR";
            break;
        case DataSourceStatusErrorKind::kErrorResponse:
            out << "ERROR_RESPONSE";
            break;
        case DataSourceStatusErrorKind::kInvalidData:
            out << "INVALID_DATA";
            break;
        case DataSourceStatusErrorKind::kStoreError:
            out << "STORE_ERROR";
            break;
    }
    return out;
}

}  // namespace launchdarkly::common::data_sources
