#include "OpenFmod/Studio/Parameter/Model.hpp"
#include <cmath>
#include <limits>
namespace OpenFmod::Studio {
Parameter ReadParameter(std::string_view bytes, const Bank::Chunk& chunk, std::uint32_t version) {
    if (version < 139 || version > 143) throw FormatError("Unsupported parameter format version");
    if (chunk.Id != "LIST" || chunk.Form != "PARM" || chunk.Children.size() != 2) {
        throw FormatError("Invalid parameter container");
    }
    Binary::Reader reader(chunk.Require("PRMB").Data(bytes));
    Parameter result;
    result.Id = Bank::ReadGuid(reader);
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Type = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Name = reader.Read(reader.Compact());
    result.Minimum = reader.Float();
    result.Maximum = reader.Float();
    result.Default = reader.Float();
    result.AuxiliaryValue = reader.Float();
    if (version < 143) {
        result.SeekSpeed = reader.Float();
        result.DescendingSeekSpeed = reader.Float();
    }
    for (const auto element : reader.Elements()) {
        Binary::Reader label(element);
        result.Labels.emplace_back(label.Read(label.Compact()));
        label.Finish();
    }
    reader.Finish();
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    if (result.Type > 9 || !std::isfinite(result.Minimum) || !std::isfinite(result.Maximum)
        || result.Maximum < result.Minimum || !std::isfinite(result.Default)
        || !std::isfinite(result.AuxiliaryValue) || !std::isfinite(result.SeekSpeed)
        || !std::isfinite(result.DescendingSeekSpeed) || result.SeekSpeed < 0 || result.DescendingSeekSpeed < 0) {
        throw FormatError("Invalid parameter settings");
    }
    if (result.Default != std::numeric_limits<float>::max()
        && (result.Default < result.Minimum || result.Default > result.Maximum)) {
        throw FormatError("Parameter default outside range");
    }
    return result;
}
ParameterLayout ReadParameterLayout(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "PMLO" || chunk.Children.size() != 2) {
        throw FormatError("Invalid parameter layout container");
    }
    Binary::Reader reader(chunk.Require("PMLB").Data(bytes));
    ParameterLayout result;
    result.Id = Bank::ReadGuid(reader);
    result.ParameterId = Bank::ReadGuid(reader);
    result.Instruments = Bank::ReadGuids(reader);
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    reader.Finish();
    Binary::Reader controllers(chunk.Require("CTRO").Data(bytes));
    result.Controllers = Bank::ReadGuids(controllers);
    controllers.Finish();
    return result;
}
}
