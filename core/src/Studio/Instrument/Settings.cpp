#include "OpenFmod/Studio/Instrument/Settings.hpp"
#include <cmath>
namespace OpenFmod::Studio {
InstrumentSettings ReadInstrumentSettings(std::string_view bytes) {
    Binary::Reader reader(bytes);
    InstrumentSettings result;
    result.Owner = Bank::ReadGuid(reader);
    result.Properties.emplace(0, reader.Float());
    result.Properties.emplace(1, reader.Float());
    result.LoopCount = static_cast<std::int32_t>(reader.Signed(4));
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Properties.emplace(7, reader.Float());
    result.Properties.emplace(8, reader.Float());
    result.MinimumDelayMilliseconds = reader.Float();
    result.MaximumDelayMilliseconds = reader.Float();
    result.QuantizationMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.QuantizationInterval = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.AutoPitchParameter = Bank::ReadGuid(reader);
    result.AutoPitchMaximum = reader.Float();
    result.Properties.emplace(9, reader.Float());
    result.MaximumInstances = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Routing = ReadRouting(reader);
    result.StealingMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.SourceOffsetTicks = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Properties.emplace(11, reader.Float());
    result.AutoPitchMinimum = reader.Float();
    const auto conditionSize = static_cast<std::size_t>(reader.Unsigned(4));
    result.Trigger = Condition(reader.Read(conditionSize));
    reader.Finish();
    for (const auto& entry : result.Properties) {
        if (!std::isfinite(entry.second)) throw FormatError("Nonfinite instrument property");
    }
    if (!std::isfinite(result.MinimumDelayMilliseconds) || !std::isfinite(result.MaximumDelayMilliseconds)
        || result.MinimumDelayMilliseconds < 0 || result.MinimumDelayMilliseconds > result.MaximumDelayMilliseconds) {
        throw FormatError("Invalid instrument delay range");
    }
    if (!std::isfinite(result.AutoPitchMinimum) || !std::isfinite(result.AutoPitchMaximum)) {
        throw FormatError("Nonfinite instrument auto pitch range");
    }
    if (result.LoopCount < -1 || !result.MaximumInstances || result.StealingMode > 2) {
        throw FormatError("Invalid instrument instance settings");
    }
    return result;
}
}
