#ifndef OPENFMOD_BINARY_CHECKSUM_HPP
#define OPENFMOD_BINARY_CHECKSUM_HPP
#include <cstdint>
#include <string_view>
namespace OpenFmod::Binary {
inline std::uint32_t Crc32(std::string_view bytes) {
    std::uint32_t result = 0xffffffff;
    for (const auto byte : bytes) {
        result ^= static_cast<unsigned char>(byte);
        for (unsigned bit = 0; bit < 8; ++bit) result = (result >> 1) ^ ((result & 1) ? 0xedb88320u : 0);
    }
    return result ^ 0xffffffff;
}
}
#endif
