#include "OpenFmod/Studio/Instrument/Definition.hpp"
#include <cmath>
namespace OpenFmod::Studio {
InstrumentDefinition ReadInstrumentDefinition(std::string_view type, std::string_view bytes) {
    Binary::Reader reader(bytes);
    InstrumentDefinition result;
    result.Id = Bank::ReadGuid(reader);
    if (type == "WAIB") {
        result.Source = WaveInstrument{Bank::ReadGuid(reader)};
    } else if (type == "MUIB") {
        result.Source = MultiInstrument{};
    } else if (type == "SLNB") {
        const auto duration = reader.Float();
        if (!std::isfinite(duration) || duration < 0) throw FormatError("Invalid silence duration");
        result.Source = SilenceInstrument{duration};
    } else if (type == "EVIB") {
        EventInstrument source;
        source.Event = Bank::ReadGuid(reader);
        source.TimelinePositionMilliseconds = reader.Float();
        if (!std::isfinite(source.TimelinePositionMilliseconds)) throw FormatError("Nonfinite event instrument position");
        for (const auto entryBytes : reader.Elements(24)) {
            Binary::Reader entry(entryBytes);
            ParameterOverride parameter;
            parameter.PropertyId = static_cast<std::uint32_t>(entry.Unsigned(4));
            parameter.Parameter = Bank::ReadGuid(entry);
            parameter.Value = entry.Float();
            if (!std::isfinite(parameter.Value)) throw FormatError("Nonfinite event instrument parameter");
            source.Parameters.push_back(parameter);
        }
        result.Source = std::move(source);
    } else if (type == "PRIB") {
        const auto length = reader.Compact();
        result.Source = ProgrammerInstrument{std::string(reader.Read(length))};
    } else if (type == "CMDB") {
        CommandInstrument source;
        source.Type = static_cast<std::uint32_t>(reader.Unsigned(4));
        source.Target = Bank::ReadGuid(reader);
        source.Value = reader.Float();
        if (!std::isfinite(source.Value)) throw FormatError("Nonfinite command instrument value");
        result.Source = source;
    } else {
        throw FormatError("Unsupported instrument definition: " + std::string(type));
    }
    reader.Finish();
    return result;
}
}
