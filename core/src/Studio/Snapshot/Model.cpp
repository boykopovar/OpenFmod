#include "OpenFmod/Studio/Snapshot/Model.hpp"
#include <cmath>
#include <set>
namespace OpenFmod::Studio {
Snapshot ReadSnapshot(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "SNAP" || chunk.Children.size() != 2) {
        throw FormatError("Invalid snapshot container");
    }
    Binary::Reader reader(chunk.Require("SNAB").Data(bytes));
    Snapshot result;
    result.Id = Bank::ReadGuid(reader);
    result.Priority = static_cast<std::int32_t>(reader.Signed(4));
    std::set<std::uint32_t> ids;
    for (const auto data : reader.Elements(28)) {
        Binary::Reader entry(data);
        SnapshotValue value;
        value.Id = static_cast<std::uint32_t>(entry.Unsigned(4));
        value.Target = Bank::ReadGuid(entry);
        value.PropertyId = static_cast<std::uint32_t>(entry.Unsigned(4));
        value.Value = entry.Float();
        if (!std::isfinite(value.Value) || !ids.insert(value.Id).second) {
            throw FormatError("Invalid snapshot property value");
        }
        result.Values.push_back(value);
    }
    result.Additive = reader.Boolean();
    result.Resolution = static_cast<SnapshotResolution>(reader.Unsigned(4));
    result.Intensity = reader.Float();
    reader.Finish();
    if ((result.Resolution != SnapshotResolution::Average && result.Resolution != SnapshotResolution::Replace)
        || !std::isfinite(result.Intensity) || result.Intensity < 0 || result.Intensity > 100) {
        throw FormatError("Invalid snapshot mix settings");
    }
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    return result;
}
}
