#ifndef OPENFMOD_STUDIO_TIMELINE_MODEL_HPP
#define OPENFMOD_STUDIO_TIMELINE_MODEL_HPP
#include "OpenFmod/Studio/Condition.hpp"
#include <string>
namespace OpenFmod::Studio {
struct ScheduledInstrument {
    Bank::Guid Instrument{};
    std::uint32_t Start{};
    std::uint32_t Duration{};
};
struct SustainPoint {
    std::uint32_t Position{};
    Condition Trigger;
};
struct NamedMarker {
    Bank::Guid Id{};
    std::uint32_t Position{};
    std::string Name;
    std::uint32_t Duration{};
};
struct TempoMarker {
    Bank::Guid Id{};
    std::uint32_t Numerator{};
    std::uint32_t Denominator{};
    std::uint32_t Position{};
    float BeatsPerMinute{};
    void Validate() const;
};
struct Timeline {
    Bank::Guid Id{};
    std::vector<ScheduledInstrument> Triggered;
    std::vector<ScheduledInstrument> TimeLocked;
    std::vector<SustainPoint> SustainPoints;
    std::vector<NamedMarker> Markers;
    std::vector<TempoMarker> Tempo;
};
Timeline ReadTimeline(std::string_view bytes);
}
#endif
