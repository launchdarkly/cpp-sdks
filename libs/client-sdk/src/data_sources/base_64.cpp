#include "launchdarkly/client_side/data_sources/detail/base_64.hpp"

#include <bitset>
#include <climits>
#include <cstddef>

#define ENCODE_SIZE 4
#define INPUT_BYTES_PER_ENCODE_SIZE 3
// Size of the index into the base64_table.
// Base64 uses a 6 bit index.
#define INDEX_BITS 6ul

namespace launchdarkly::client_side {

// URL safe base64 table.
static unsigned char const kBase64Table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/**
 * Get a bit set populated with the bits at the specific start_bit through
 * the count.
 */
static std::bitset<INDEX_BITS> GetBits(std::size_t start_bit,
                                       std::size_t count,
                                       std::string const& input) {
    std::bitset<INDEX_BITS> out_set;
    auto out_index = 0;
    // Iterate the bits from the highest bit. bit 0, would be the 7th
    // bit in the first byte.
    for (auto bit_index = start_bit; bit_index < start_bit + count;
         bit_index++) {
        auto str_index = bit_index / CHAR_BIT;
        auto character = input[str_index];
        size_t bit_in_byte = (CHAR_BIT - 1) - (bit_index % CHAR_BIT);
        unsigned char bit_mask = 1 << (bit_in_byte);
        auto bit = !!(bit_mask & character);
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
    auto reserve_size = (input.size() / INPUT_BYTES_PER_ENCODE_SIZE) * ENCODE_SIZE;
    // If not a multiple of 3, then we need to add 4 more bytes to the size.
    // This will contain the extra encoded characters and padding.
    if(input.size() % INPUT_BYTES_PER_ENCODE_SIZE) {
        reserve_size += ENCODE_SIZE;
    }
    out.reserve(reserve_size);

    while (bit_index < bit_count) {
        // Get either 6 bits, or the remaining number of bits.
        auto bits = GetBits(bit_index,
                            std::min(INDEX_BITS, bit_count - bit_index), input);
        out.push_back(kBase64Table[bits.to_ulong()]);
        bit_index += INDEX_BITS;
    }
    // If the string is not divisible evenly by the ENCODE_SIZE
    // then pad it with '=' until it is.
    if (out.size() % ENCODE_SIZE != 0) {
        out.append(ENCODE_SIZE - (out.size()) % ENCODE_SIZE, '=');
    }
    return out;
}

}  // namespace launchdarkly::client_side
