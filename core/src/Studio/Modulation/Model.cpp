#include "OpenFmod/Studio/Modulation/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
float readFinite(Binary::Reader& reader) {
    const auto value = reader.Float();
    if (!std::isfinite(value)) throw FormatError("Nonfinite modulation parameter");
    return value;
}
float readDuration(Binary::Reader& reader) {
    const auto value = readFinite(reader);
    if (value < 0) throw FormatError("Negative modulation duration");
    return value;
}
Envelope readEnvelope(Binary::Reader& reader) {
    Envelope result;
    result.Initial = readFinite(reader);
    result.Peak = readFinite(reader);
    result.Sustain = readFinite(reader);
    result.AttackMilliseconds = readDuration(reader);
    result.HoldMilliseconds = readDuration(reader);
    result.DecayMilliseconds = readDuration(reader);
    result.ReleaseMilliseconds = readDuration(reader);
    for (auto& value : result.Curvature) value = readFinite(reader);
    result.Final = readFinite(reader);
    return result;
}
EnvelopeFollower readFollower(Binary::Reader& reader) {
    EnvelopeFollower result;
    result.Amount = readFinite(reader);
    result.MinimumDecibels = readFinite(reader);
    result.MaximumDecibels = readFinite(reader);
    result.AttackMilliseconds = readDuration(reader);
    result.ReleaseMilliseconds = readDuration(reader);
    const auto rms = reader.Unsigned(1);
    if (rms > 1 || result.MinimumDecibels > result.MaximumDecibels) throw FormatError("Invalid envelope follower parameters");
    result.RootMeanSquare = rms != 0;
    return result;
}
}
Modulation ReadModulation(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Binary::Reader header(reader.Read(static_cast<std::size_t>(reader.Unsigned(2))));
    Modulation result;
    result.Id = Bank::ReadGuid(header);
    result.Owner = Bank::ReadGuid(header);
    result.PropertyId = static_cast<std::uint32_t>(header.Unsigned(4));
    const auto type = header.Unsigned(4);
    result.ValueMode = static_cast<std::uint32_t>(header.Unsigned(4));
    if (result.ValueMode > 2) throw FormatError("Invalid modulation value mode");
    header.Finish();
    switch (type) {
        case 0: result.Parameters = readEnvelope(reader); break;
        case 1: result.Parameters = RandomModulation{readFinite(reader)}; break;
        case 2: result.Parameters = readFollower(reader); break;
        default: throw FormatError("Unsupported modulation type: " + std::to_string(type));
    }
    reader.Finish();
    return result;
}
}
