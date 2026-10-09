#include "OpenFmod/Studio/Timeline/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
namespace {
std::vector<ScheduledInstrument> readInstruments(Binary::Reader& reader) {
    std::vector<ScheduledInstrument> result;
    for (const auto bytes : reader.Elements(24)) {
        Binary::Reader entry(bytes);
        ScheduledInstrument instrument;
        instrument.Instrument = Bank::ReadGuid(entry);
        instrument.Start = static_cast<std::uint32_t>(entry.Unsigned(4));
        instrument.Duration = static_cast<std::uint32_t>(entry.Unsigned(4));
        entry.Finish();
        result.push_back(instrument);
    }
    return result;
}
}
Timeline ReadTimeline(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Timeline result;
    result.Id = Bank::ReadGuid(reader);
    result.Triggered = readInstruments(reader);
    result.TimeLocked = readInstruments(reader);
    for (const auto data : reader.Elements()) {
        Binary::Reader entry(data);
        const auto position = static_cast<std::uint32_t>(entry.Unsigned(4));
        const auto size = static_cast<std::size_t>(entry.Unsigned(4));
        result.SustainPoints.push_back({position, Condition(entry.Read(size))});
        entry.Finish();
    }
    for (const auto data : reader.Elements()) {
        Binary::Reader entry(data);
        NamedMarker marker;
        marker.Id = Bank::ReadGuid(entry);
        marker.Position = static_cast<std::uint32_t>(entry.Unsigned(4));
        marker.Name = std::string(entry.Read(entry.Compact()));
        marker.Flags = static_cast<std::uint32_t>(entry.Unsigned(4));
        entry.Finish();
        result.Markers.push_back(std::move(marker));
    }
    for (const auto data : reader.Elements(32)) {
        Binary::Reader entry(data);
        TempoMarker marker;
        marker.Id = Bank::ReadGuid(entry);
        marker.Numerator = static_cast<std::uint32_t>(entry.Unsigned(4));
        marker.Denominator = static_cast<std::uint32_t>(entry.Unsigned(4));
        marker.Position = static_cast<std::uint32_t>(entry.Unsigned(4));
        marker.BeatsPerMinute = entry.Float();
        if (!marker.Numerator || !marker.Denominator || !std::isfinite(marker.BeatsPerMinute) || marker.BeatsPerMinute <= 0) {
            throw FormatError("Invalid timeline tempo marker");
        }
        entry.Finish();
        result.Tempo.push_back(marker);
    }
    reader.Finish();
    return result;
}
}
