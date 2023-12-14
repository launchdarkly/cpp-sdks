#pragma once

#include <cstdint>
#include <limits>
#include <ostream>
#include <variant>
#include <string>

namespace launchdarkly {
enum class ErrorCode : std::uint32_t {
    KReserved1 = 0,
    KReserved2 = 1,
    /* Common lib errors: 2-9999 */
    kConfig_Endpoints_EmptyURL = 100,
    kConfig_Endpoints_AllURLsMustBeSet = 101,

    kConfig_ApplicationInfo_EmptyKeyOrValue = 200,
    kConfig_ApplicationInfo_ValueTooLong = 201,
    kConfig_ApplicationInfo_InvalidKeyCharacters = 202,
    kConfig_ApplicationInfo_InvalidValueCharacters = 203,

    kConfig_Events_ZeroCapacity = 300,

    kConfig_SDKKey_Empty = 400,
    /* Client-side errors: 10000-19999 */
    /* Server-side errors: 20000-29999 */
    kConfig_DataSystem_LazyLoad_MissingSource = 20000,
    kMax = std::numeric_limits<std::uint32_t>::max()
};

using Error = std::variant<ErrorCode, std::string>;

bool operator==(Error const& lhs, ErrorCode const& rhs);


char const* ErrorToString(Error const& err);
std::ostream& operator<<(std::ostream& os, Error const& err);
} // namespace launchdarkly
