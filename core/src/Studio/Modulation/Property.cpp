#include "OpenFmod/Studio/Modulation/Property.hpp"
namespace OpenFmod::Studio {
Property ReadProperty(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Property result;
    result.Id = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Type = static_cast<std::uint16_t>(reader.Unsigned(2));
    result.Flags = static_cast<std::uint16_t>(reader.Unsigned(2));
    result.Source = Bank::ReadGuid(reader);
    for (auto* collection : {&result.Curves, &result.Modulators}) {
        for (const auto element : reader.Elements(16)) {
            Binary::Reader entry(element);
            collection->push_back(Bank::ReadGuid(entry));
        }
    }
    reader.Finish();
    return result;
}
}
