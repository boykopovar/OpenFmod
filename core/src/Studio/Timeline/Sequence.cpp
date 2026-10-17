#include "OpenFmod/Studio/Timeline/Sequence.hpp"
namespace OpenFmod::Studio {
std::vector<ScheduledInstrument> ReadSchedule(Binary::Reader& reader) {
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
namespace {
std::vector<CurveBinding> readBindings(Binary::Reader& reader) {
    std::vector<CurveBinding> result;
    for (const auto bytes : reader.Elements(32)) {
        Binary::Reader entry(bytes);
        result.push_back({Bank::ReadGuid(entry), Bank::ReadGuid(entry)});
    }
    return result;
}
}
TransitionSequence ReadTransitionSequence(std::string_view bytes) {
    Binary::Reader reader(bytes);
    TransitionSequence result;
    result.Duration = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.Curves = readBindings(reader);
    result.Triggered = ReadSchedule(reader);
    result.TimeLocked = ReadSchedule(reader);
    result.SourceOffset = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.DestinationOffset = static_cast<std::uint32_t>(reader.Unsigned(4));
    result.SourceFades = readBindings(reader);
    result.DestinationFades = readBindings(reader);
    result.FadeMapping = Bank::ReadGuid(reader);
    result.CurveReplacements = readBindings(reader);
    reader.Finish();
    return result;
}
}
