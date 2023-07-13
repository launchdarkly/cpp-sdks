#pragma once

#include <array>
#include <iomanip>
#include <sstream>
#include <string>

namespace launchdarkly::encoding {

template <unsigned long N>
std::string Base16Encode(std::array<unsigned char, N> arr) {
    std::stringstream output_stream;
    output_stream << std::hex << std::noshowbase;
    for (unsigned char byte : arr) {
        output_stream << std::setw(2) << std::setfill('0')
                      << static_cast<int>(byte);
    }
    return output_stream.str();
}

}  // namespace launchdarkly::encoding
