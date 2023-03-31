#pragma once

#include <cstdint>
#include <limits>

namespace launchdarkly {

enum class Error : std::uint32_t {
    KReserved1 = 0,
    KReserved2 = 1,
    /* Common lib errors: 2-99 */
    kConfig_Endpoints_EmptyURL = 2,
    kConfig_Endpoints_AllURLsMustBeSet = 3,
    /* Client-side errors: 100-199 */
    /* Server-side errors: 200-299 */

    kMax = std::numeric_limits<std::uint32_t>::max()
};

}  // namespace launchdarkly
