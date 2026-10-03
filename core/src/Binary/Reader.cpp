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
