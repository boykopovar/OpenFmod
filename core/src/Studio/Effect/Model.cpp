#include "OpenFmod/Studio/Effect/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
EffectParameter readParameter(Binary::Reader& reader) {
    const auto type = reader.Unsigned(4);
    switch (type) {
        case 0: {
            const auto value = reader.Float();
            if (!std::isfinite(value)) throw FormatError("Nonfinite effect parameter");
            return value;
        }
        case 1: return static_cast<std::int32_t>(reader.Signed(4));
        case 2: return reader.Boolean();
        case 3: return std::string(reader.Read(reader.Compact()));
        default: throw FormatError("Unsupported effect parameter type");
    }
}
}
Effect ReadEffect(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "BEFF" || chunk.Children.size() != 3) {
        throw FormatError("Invalid built-in effect container");
    }
    Effect result;
    Binary::Reader header(chunk.Require("BEFB").Data(bytes));
    result.Id = Bank::ReadGuid(header);
    result.Type = static_cast<std::uint32_t>(header.Unsigned(4));
    result.LegacyBypass = header.Boolean();
    header.Finish();
    if (result.Type >= 38) throw FormatError("Invalid built-in effect type");
    Binary::Reader reader(chunk.Require("PMEF").Data(bytes));
    const auto count = reader.Unsigned(4);
    if (count > reader.Remaining() / 5) throw FormatError("Truncated effect parameters");
    result.Parameters.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index) result.Parameters.push_back(readParameter(reader));
    result.ParameterFlag = reader.Boolean();
    reader.Finish();
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    return result;
}
Sidechain ReadSidechain(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "SCFF" || chunk.Children.size() != 2) {
        throw FormatError("Invalid sidechain container");
    }
    Binary::Reader reader(chunk.Require("SCEF").Data(bytes));
    Sidechain result;
    result.Id = Bank::ReadGuid(reader);
    result.LegacyFlag = reader.Boolean();
    result.Effects = Bank::ReadGuids(reader);
    result.Followers = Bank::ReadGuids(reader);
    result.GainDecibels = reader.Float();
    reader.Finish();
    if (!std::isfinite(result.GainDecibels)) throw FormatError("Nonfinite sidechain gain");
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    return result;
}

}
