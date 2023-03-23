#pragma once

#include <ostream>

/**
 * Simple class for verifying an ostream operator is or is not called.
 */
class OstreamTester {
   public:
    bool& was_converted;
    OstreamTester(bool& was_converted) : was_converted(was_converted){};
    friend std::ostream& operator<<(std::ostream& os,
                                    OstreamTester const& tester) {
        tester.was_converted = true;
        return os;
    }
};
