#include "OpenFmod/Bank/Container.hpp"
#include "OpenFmod/Binary/Reader.hpp"
namespace OpenFmod::Bank {
namespace {
std::vector<Chunk> readChunks(std::string_view bytes, std::size_t base, unsigned depth) {
    if (depth > 64) throw FormatError("Excessive bank container nesting");
    Binary::Reader reader(bytes);
    std::vector<Chunk> result;
    while (reader.Remaining()) {
        Chunk chunk;
        chunk.Id = std::string(reader.Read(4));
        chunk.Size = static_cast<std::size_t>(reader.Unsigned(4));
        chunk.Offset = base + reader.Position();
        const auto payload = reader.Read(chunk.Size);
        if (chunk.Id == "LIST") {
            Binary::Reader list(payload);
            chunk.Form = std::string(list.Read(4));
            chunk.Children = readChunks(list.Read(list.Remaining()), chunk.Offset + 4, depth + 1);
        }
        if (chunk.Size & 1) reader.Read(1);
        result.push_back(std::move(chunk));
    }
    return result;
}
}
const Chunk& Chunk::Require(std::string_view id, std::string_view form) const {
    const Chunk* found = nullptr;
    for (const auto& child : Children) {
        if (child.Id != id || child.Form != form) continue;
        if (found) throw FormatError("Duplicate bank chunk: " + std::string(id) + std::string(form));
        found = &child;
    }
    if (!found) throw FormatError("Missing bank chunk: " + std::string(id) + std::string(form));
    return *found;
}
Chunk ReadContainer(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Chunk result;
    result.Id = std::string(reader.Read(4));
    if (result.Id != "RIFF") throw FormatError("Expected a little endian RIFF bank");
    result.Size = static_cast<std::size_t>(reader.Unsigned(4));
    result.Offset = reader.Position();
    if (result.Size != reader.Remaining()) throw FormatError("RIFF bank length does not match its header");
    result.Form = std::string(reader.Read(4));
    if (result.Form != "FEV ") throw FormatError("Expected an FEV bank");
    result.Children = readChunks(reader.Read(reader.Remaining()), 12, 0);
    return result;
}
}
