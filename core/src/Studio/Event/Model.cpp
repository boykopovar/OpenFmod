#include "OpenFmod/Studio/Event/Model.hpp"
#include <cmath>
namespace OpenFmod::Studio {
Event ReadEvent(std::string_view bytes) {
    Binary::Reader reader(bytes);
    Event result;
    result.Id = Bank::ReadGuid(reader);
    result.Snapshot = Bank::ReadGuid(reader);
    result.Timeline = Bank::ReadGuid(reader);
    result.InputBus = Bank::ReadGuid(reader);
    result.MasterBus = Bank::ReadGuid(reader);
    result.MaximumInstances = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Priority = static_cast<std::uint32_t>(reader.Unsigned(4));
    const auto legacy = reader.Unsigned(1);
    if (legacy > 1) throw FormatError("Invalid event legacy stealing flag");
    result.LegacyRejectNew = legacy != 0;
    result.SchedulingMode = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.ParameterLayouts = Bank::ReadGuids(reader);
    for (const auto bytes : reader.Elements()) {
        Binary::Reader entry(bytes);
        const std::string name(entry.Read(entry.Compact()));
        const auto value = entry.Float();
        if (!result.NumericProperties.emplace(name, value).second) throw FormatError("Duplicate numeric event property");
        entry.Finish();
    }
    for (const auto bytes : reader.Elements()) {
        Binary::Reader entry(bytes);
        const std::string name(entry.Read(entry.Compact()));
        const std::string value(entry.Read(entry.Compact()));
        if (!result.StringProperties.emplace(name, value).second) throw FormatError("Duplicate string event property");
        entry.Finish();
    }
    result.DopplerPercent = reader.Float();
    result.Stealing = static_cast<StealingMode>(reader.Unsigned(4));
    result.CooldownSeconds = reader.Float();
    result.Flags = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Groups = Bank::ReadGuids(reader);
    for (const auto bytes : reader.Elements(8)) {
        Binary::Reader entry(bytes);
        result.ParameterIds.push_back(entry.Unsigned(8));
    }
    result.Instruments = Bank::ReadGuids(reader);
    result.MinimumDistance = reader.Float();
    result.MaximumDistance = reader.Float();
    if (!result.MaximumInstances || result.Priority > 256 || result.Stealing > StealingMode::Furthest
        || result.SchedulingMode > 4) throw FormatError("Invalid event playback settings");
    if (!std::isfinite(result.DopplerPercent) || result.DopplerPercent < 0
        || !std::isfinite(result.CooldownSeconds) || result.CooldownSeconds < 0
        || !std::isfinite(result.MinimumDistance) || !std::isfinite(result.MaximumDistance)
        || result.MinimumDistance < 0 || result.MaximumDistance < result.MinimumDistance) {
        throw FormatError("Invalid event scalar settings");
    }
    reader.Finish();
    return result;
}
}
