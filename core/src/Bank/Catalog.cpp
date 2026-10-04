#include "OpenFmod/Bank/Catalog.hpp"
#include "OpenFmod/Binary/Reader.hpp"
#include <unordered_set>
namespace OpenFmod::Bank {
namespace {
constexpr std::uint32_t noIndex = 0xffffff;
struct Node {
    std::uint32_t Key;
    std::uint32_t Link;
};
std::vector<std::uint32_t> readIndices(Binary::Reader& reader, std::size_t expected) {
    if (reader.Compact() != expected) throw FormatError("Catalog index count mismatch");
    if (expected > reader.Remaining() / 3) throw FormatError("Truncated catalog indices");
    std::vector<std::uint32_t> result;
    result.reserve(expected);
    for (std::size_t index = 0; index < expected; ++index) {
        result.push_back(static_cast<std::uint32_t>(reader.Unsigned(3)));
    }
    return result;
}
}
std::vector<CatalogEntry> ReadCatalog(std::string_view bytes) {
    Binary::Reader reader(bytes);
    if (reader.Unsigned(4) != 1) throw FormatError("Unsupported catalog encoding");
    std::vector<Node> nodes;
    for (const auto element : reader.Elements(8)) {
        Binary::Reader node(element);
        nodes.push_back({static_cast<std::uint32_t>(node.Unsigned(4)), static_cast<std::uint32_t>(node.Unsigned(4))});
    }
    std::vector<CatalogEntry> entries;
    for (const auto element : reader.Elements(16)) {
        CatalogEntry entry;
        Binary::Reader identity(element);
        entry.Id = ReadGuid(identity);
        entries.push_back(std::move(entry));
    }
    const auto blobSize = reader.Compact();
    const auto blob = reader.Read(blobSize);
    const auto leaves = readIndices(reader, entries.size());
    const auto parents = readIndices(reader, nodes.size());
    reader.Finish();
    if (nodes.empty()) {
        if (!entries.empty() || !blob.empty()) throw FormatError("Missing catalog tree");
        return entries;
    }
    if (parents.front() != noIndex) throw FormatError("Invalid catalog root parent");
    std::vector<std::string> paths(nodes.size());
    std::vector<bool> reached(nodes.size());
    reached.front() = true;
    std::unordered_set<std::string> uniquePaths;
    std::size_t leafCount = 0;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (!reached[index]) throw FormatError("Disconnected catalog node");
        const auto& node = nodes[index];
        const auto offset = node.Key & noIndex;
        const auto prefix = node.Key >> 24;
        if (index) paths[index] = paths[parents[index]];
        if (offset == noIndex) {
            if (prefix) throw FormatError("Invalid empty catalog prefix");
        } else {
            if (!prefix || offset >= blob.size()) throw FormatError("Invalid catalog string offset");
            const auto end = blob.find('\0', offset);
            if (end == std::string_view::npos) throw FormatError("Unterminated catalog string");
            paths[index].append(blob.substr(offset, end - offset));
        }
        const auto first = node.Link & noIndex;
        const auto count = node.Link >> 24;
        if (!count) {
            if (first >= entries.size() || leaves[first] != index) throw FormatError("Invalid catalog leaf");
            if (!uniquePaths.insert(paths[index]).second) throw FormatError("Duplicate catalog path");
            entries[first].Path = paths[index];
            ++leafCount;
        } else {
            if (first <= index || first > nodes.size() || count > nodes.size() - first) throw FormatError("Invalid catalog children");
            for (std::size_t child = first; child < first + count; ++child) {
                if (reached[child] || parents[child] != index) throw FormatError("Invalid catalog parent link");
                reached[child] = true;
            }
        }
    }
    if (leafCount != entries.size()) throw FormatError("Unreferenced catalog entry");
    return entries;
}
}
