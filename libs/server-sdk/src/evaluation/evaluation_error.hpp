#pragma once

namespace launchdarkly::server_side::evaluation {

enum Error {
    kCyclicReference,
    kBigSegmentEncountered,
    kInvalidAttributeReference,
    kRolloutMissingVariations
};

}
