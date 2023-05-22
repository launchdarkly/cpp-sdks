#pragma once

namespace launchdarkly::sse {

enum class Error {
    NoContent = 1,
    InvalidRedirectLocation = 2,
    UnrecoverableClientError = 3,
};
}
