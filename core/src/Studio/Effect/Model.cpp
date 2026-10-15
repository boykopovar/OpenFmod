#include "OpenFmod/Studio/Effect/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
bool readBool(Binary::Reader& reader) {
    const auto value = reader.Unsigned(1);
    if (value > 1) throw FormatError("Invalid effect boolean");
    return value != 0;
}
EffectParameter readParameter(Binary::Reader& reader) {
    const auto type = reader.Unsigned(4);
    switch (type) {
        case 0: {
            const auto value = reader.Float();
            if (!std::isfinite(value)) throw FormatError("Nonfinite effect parameter");
            return value;
        }
        case 1: return static_cast<std::int32_t>(reader.Signed(4));
        case 2: return readBool(reader);
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
    result.LegacyBypass = readBool(header);
    header.Finish();
    if (result.Type >= 38) throw FormatError("Invalid built-in effect type");
    Binary::Reader reader(chunk.Require("PMEF").Data(bytes));
    const auto count = reader.Unsigned(4);
    if (count > reader.Remaining() / 5) throw FormatError("Truncated effect parameters");
    result.Parameters.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index) result.Parameters.push_back(readParameter(reader));
    result.ParameterFlag = readBool(reader);
    reader.Finish();
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    return result;
}
}
