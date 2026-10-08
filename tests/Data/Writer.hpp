#ifndef OPENFMOD_TESTS_DATA_WRITER_HPP
#define OPENFMOD_TESTS_DATA_WRITER_HPP
#include <cstdint>
#include <cstring>
#include <string>
namespace TestData {
inline void Append(std::string& bytes, std::uint64_t value, unsigned width) {
    for (unsigned index = 0; index < width; ++index) bytes += static_cast<char>((value >> (index * 8)) & 255);
}
inline void AppendFloat(std::string& bytes, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    Append(bytes, bits, 4);
}
}
#endif
