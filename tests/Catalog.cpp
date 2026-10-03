#include "OpenFmod/Bank/Catalog.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <stdexcept>
#include <string>
namespace {
void append(std::string& bytes, std::uint32_t value, unsigned width) {
    for (unsigned index = 0; index < width; ++index) bytes += static_cast<char>((value >> (index * 8)) & 255);
}
std::string catalog() {
    std::string bytes;
    append(bytes, 1, 4);
    append(bytes, 5, 2);
    append(bytes, 8, 2);
    append(bytes, 0xffffff, 4);
    append(bytes, 0x1000001, 4);
    append(bytes, 0x65000000, 4);
    append(bytes, 0, 4);
    append(bytes, 3, 2);
    append(bytes, 16, 2);
    bytes.append(16, '\x12');
    const std::string path = "event:/synthetic/test";
    append(bytes, static_cast<std::uint32_t>(path.size() + 1), 2);
    bytes += path;
    bytes += '\0';
    append(bytes, 1, 2);
    append(bytes, 1, 3);
    append(bytes, 2, 2);
    append(bytes, 0xffffff, 3);
    append(bytes, 0, 3);
    return bytes;
}
}
void CheckCatalog() {
    auto bytes = catalog();
    const auto entries = OpenFmod::Bank::ReadCatalog(bytes);
    if (entries.size() != 1 || entries[0].Path != "event:/synthetic/test" || entries[0].Id[15] != 0x12) {
        throw std::runtime_error("Catalog path or identity mismatch");
    }
    bytes.back() = '\x01';
    bool rejected = false;
    try { OpenFmod::Bank::ReadCatalog(bytes); }
    catch (const OpenFmod::FormatError&) { rejected = true; }
    if (!rejected) throw std::runtime_error("Invalid catalog parent accepted");
    bytes = catalog();
    for (std::size_t length = 0; length < bytes.size(); ++length) {
        rejected = false;
        try { OpenFmod::Bank::ReadCatalog(std::string_view(bytes).substr(0, length)); }
        catch (const OpenFmod::FormatError&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Truncated catalog accepted");
    }
    std::string compact;
    append(compact, 0x8001, 2);
    append(compact, 2, 2);
    OpenFmod::Binary::Reader reader(compact);
    if (reader.Compact() != 65537) throw std::runtime_error("Extended integer mismatch");
    reader.Finish();
}
