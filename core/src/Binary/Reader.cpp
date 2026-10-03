#include "OpenFmod/Binary/Reader.hpp"
#include <cstring>
#include <limits>
#include <string>
namespace OpenFmod::Binary {
Reader::Reader(std::string_view bytes) : bytes(bytes) {}
std::size_t Reader::Position() const { return position; }
std::size_t Reader::Remaining() const { return bytes.size() - position; }
std::string_view Reader::Read(std::size_t size) {
    if (size > Remaining()) throw FormatError("Truncated data at byte " + std::to_string(position));
    const auto result = bytes.substr(position, size);
    position += size;
    return result;
}
std::uint64_t Reader::Unsigned(std::size_t size) {
    if (!size || size > 8) throw FormatError("Invalid integer width");
    const auto source = Read(size);
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < size; ++index) {
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(source[index])) << (index * 8);
    }
    return value;
}
std::uint32_t Reader::Compact() {
    const auto low = static_cast<std::uint32_t>(Unsigned(2));
    if (!(low & 0x8000)) return low;
    return (low & 0x7fff) | (static_cast<std::uint32_t>(Unsigned(2)) << 15);
}
std::vector<std::string_view> Reader::Elements(std::size_t elementSize) {
    const auto header = Compact();
    const auto count = header >> 1;
    if (!elementSize || count > Remaining() / elementSize) throw FormatError("Invalid element array length");
    std::vector<std::string_view> result;
    result.reserve(count);
    std::size_t size = 0;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (!index || !(header & 1)) size = static_cast<std::size_t>(Unsigned(2));
        if (size != elementSize) throw FormatError("Unexpected element size: " + std::to_string(size));
        result.push_back(Read(size));
    }
    return result;
}
float Reader::Float() {
    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    const auto bits = static_cast<std::uint32_t>(Unsigned(4));
    float result;
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}
void Reader::Finish() const {
    if (Remaining()) throw FormatError("Unexpected trailing data at byte " + std::to_string(position));
}
}
