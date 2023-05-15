#include <launchdarkly/client_side/data_sources/detail/base_64.hpp>

#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cstddef>

static unsigned char const kEncodeSize = 4;
static unsigned char const kInputBytesPerEncodeSize = 3;

// Size of the index into the base64_table.
// Base64 uses a 6 bit index.
static unsigned long const kIndexBits = 6UL;

namespace launchdarkly::client_side::data_sources::detail {

// URL safe base64 table.
static std::array<unsigned char, 65> const kBase64Table{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

/**
 * Get a bit set populated with the bits at the specific start_bit through
 * the count.
 */
static std::bitset<kIndexBits> GetBits(std::size_t start_bit,
                                       std::size_t count,
                                       std::string const& input) {
    std::bitset<kIndexBits> out_set;
    auto out_index = 0;
    // Iterate the bits from the highest bit. bit 0, would be the 7th
    // bit in the first byte.
    for (auto bit_index = start_bit; bit_index < start_bit + count;
         bit_index++) {
        auto str_index = bit_index / CHAR_BIT;
        auto character = input[str_index];
        size_t bit_in_byte = (CHAR_BIT - 1) - (bit_index % CHAR_BIT);
        unsigned char bit_mask = 1 << (bit_in_byte);
        auto bit = (bit_mask & character) != 0;
        out_set[out_set.size() - 1 - out_index] = bit;
        out_index++;
    }
    return out_set;
}

std::string Base64UrlEncode(std::string const& input) {
    auto bit_count = input.size() * CHAR_BIT;
    std::string out;
    std::size_t bit_index = 0;

    // Every 3 bytes takes 4 characters of output.
    auto reserve_size = (input.size() / kInputBytesPerEncodeSize) * kEncodeSize;
    // If not a multiple of 3, then we need to add 4 more bytes to the size.
    // This will contain the extra encoded characters and padding.
    if ((input.size() % kInputBytesPerEncodeSize) != 0U) {
        reserve_size += kEncodeSize;
    }
    out.reserve(reserve_size);

    while (bit_index < bit_count) {
        // Get either 6 bits, or the remaining number of bits.
        auto bits =
            GetBits(bit_index,
                    std::min(kIndexBits,
                             static_cast<unsigned long>(bit_count - bit_index)),
                    input);
        out.push_back(static_cast<char>(kBase64Table.at(bits.to_ulong())));
        bit_index += kIndexBits;
    }
    // If the string is not divisible evenly by the kEncodeSize
    // then pad it with '=' until it is.
    if (out.size() % kEncodeSize != 0) {
        out.append(kEncodeSize - (out.size()) % kEncodeSize, '=');
    }
    return out;
}

}  // namespace launchdarkly::client_side::data_sources::detail
