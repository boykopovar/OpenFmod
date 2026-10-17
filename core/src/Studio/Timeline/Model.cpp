#include "OpenFmod/Studio/Timeline/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
void TempoMarker::Validate() const {
    if (!Numerator || !Denominator || Denominator > 16 || !std::isfinite(BeatsPerMinute) || BeatsPerMinute < 0) {
        throw FormatError("Invalid timeline tempo marker");
    }
}
Timeline ReadTimeline(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Timeline result;
    result.Id = Bank::ReadGuid(reader);
    result.Triggered = ReadSchedule(reader);
    result.TimeLocked = ReadSchedule(reader);
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
        marker.Duration = static_cast<std::uint32_t>(entry.Unsigned(4));
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
        marker.Validate();
        entry.Finish();
        result.Tempo.push_back(marker);
    }
    reader.Finish();
    return result;
}
}
