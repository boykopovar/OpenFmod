#include "OpenFmod/Studio/Bus/Controls.hpp"
#include <cmath>
namespace OpenFmod::Studio {
MixControls ReadMixControls(Binary::Reader& reader) {
    MixControls result;
    Binary::Reader levels(reader.Read(static_cast<std::size_t>(reader.Unsigned(2))));
    result.VolumeDecibels = levels.Float();
    result.PitchSemitones = levels.Float();
    levels.Finish();
    result.VolumeControls = Bank::ReadGuids(reader);
    if (!std::isfinite(result.VolumeDecibels) || !std::isfinite(result.PitchSemitones)) {
        throw FormatError("Nonfinite mix control value");
    }
    return result;
}
VolumeControl ReadVolumeControl(std::string_view bytes, const Bank::Chunk& chunk) {
    if (chunk.Id != "LIST" || chunk.Form != "VCA " || chunk.Children.size() != 2) {
        throw FormatError("Invalid volume control container");
    }
    VolumeControl result;
    Binary::Reader reader(chunk.Require("VCAB").Data(bytes));
    result.Id = Bank::ReadGuid(reader);
    result.Targets = Bank::ReadGuids(reader);
    result.Levels = ReadMixControls(reader);
    reader.Finish();
    result.Properties = ReadProperties(bytes, chunk.Require("LIST", "PRPS"));
    return result;
}
}
