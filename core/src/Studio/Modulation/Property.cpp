#include "OpenFmod/Studio/Modulation/Property.hpp"
namespace OpenFmod::Studio {
Property ReadProperty(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Property result;
    result.Id = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Resolution = static_cast<std::uint16_t>(reader.Unsigned(2));
    result.ValueType = static_cast<std::uint16_t>(reader.Unsigned(2));
    result.MappingId = Bank::ReadGuid(reader);
    if (result.Resolution >= 7 || result.ValueType >= 3) throw FormatError("Invalid property resolution or value type");
    result.Curves = Bank::ReadGuids(reader);
    result.Modulators = Bank::ReadGuids(reader);
    reader.Finish();
    return result;
}
std::map<std::uint32_t, Property> ReadProperties(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "PRPS") throw FormatError("Expected property collection");
    chunk.ValidateCount(bytes);
    std::map<std::uint32_t, Property> result;
    for (const auto& child : chunk.Children) {
        if (child.Id == "LCNT") continue;
        if (child.Id != "PROP" || !child.Form.empty()) throw FormatError("Unexpected property collection entry");
        auto property = ReadProperty(child.Data(bytes));
        const auto id = property.Id;
        if (!result.emplace(id, std::move(property)).second) throw FormatError("Duplicate property identity");
    }
    return result;
}
}
