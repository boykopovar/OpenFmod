#ifndef OPENFMOD_BINARY_READER_HPP
#define OPENFMOD_BINARY_READER_HPP
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>
namespace OpenFmod {
class FormatError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
namespace Binary {
class Reader {
public:
    explicit Reader(std::string_view bytes);
    std::size_t Position() const;
    std::size_t Remaining() const;
    std::string_view Read(std::size_t size);
    std::uint64_t Unsigned(std::size_t size);
    std::int64_t Signed(std::size_t size);
    std::uint32_t Compact();
    std::vector<std::string_view> Elements(std::optional<std::size_t> elementSize = std::nullopt);
    float Float();
    bool Boolean();
    void Finish() const;
private:
    std::string_view bytes;
    std::size_t position{};
};
}
}
#endif
