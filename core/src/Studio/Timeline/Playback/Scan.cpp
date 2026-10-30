#include "OpenFmod/Studio/Timeline/Playback/Scan.hpp"
#include "OpenFmod/Studio/Timeline/Playback/Bounds.hpp"
#include <algorithm>
#include <cmath>
namespace OpenFmod::Studio {
TimelineScan::TimelineScan(const TimelineResource& timeline) : timeline(timeline), tempo(timeline.Definition.Tempo) {
    for (const auto& item : timeline.Transitions) {
        if (item.Start > item.End || item.Flags > 31 || item.QuantizationMode > 2
            || !std::isfinite(item.Probability) || item.Probability < 0 || item.Probability > 100) throw FormatError("Invalid timeline transition");
    }
}
void TimelineScan::Reset() { rejected.clear(); }
bool TimelineScan::roll(float probability, const std::function<float()>& random) {
    if (probability == 0) return false;
    if (probability == 100) return true;
    if (!random) throw FormatError("Missing timeline random source");
    const auto value = random();
    if (!std::isfinite(value) || value < 0 || value >= 1) throw FormatError("Invalid timeline random value");
    return probability > value * 100;
}
std::optional<std::uint32_t> TimelineScan::candidate(std::size_t index, std::uint32_t begin, std::uint32_t end, const std::function<float()>& random) {
    const auto& region = timeline.Transitions[index];
    if (region.Probability == 0) return std::nullopt;
    auto position = std::max(begin, region.Start);
    if (!region.QuantizationMode) {
        if (!rejected.count(index) && roll(region.Probability, random)) return position;
        if (region.Start != region.End || (region.Flags & 16)) rejected.insert(index);
        return std::nullopt;
    }
    const auto limit = (region.Flags & 16) ? end : std::min(end, region.End);
    auto tick = tempo.Quantize(position, static_cast<QuantizationMode>(region.QuantizationMode), region.QuantizationInterval);
    while (tick < limit) {
        if (roll(region.Probability, random)) return static_cast<std::uint32_t>(tick);
        tick = tempo.Quantize(tick + 1, static_cast<QuantizationMode>(region.QuantizationMode), region.QuantizationInterval);
    }
    return std::nullopt;
}
std::optional<LogicPoint> TimelineScan::Find(const TimelineWindow& window, const std::function<float(const Bank::Guid&)>& parameter, const std::function<float()>& random) {
    if (window.Begin > window.End) throw FormatError("Reversed timeline scan window");
    std::optional<LogicPoint> result;
    auto keyOffs = window.KeyOffs;
    if (!window.LeadsOnly) {
        for (std::size_t index = 0; index < timeline.Definition.SustainPoints.size(); ++index) {
            const auto& sustain = timeline.Definition.SustainPoints[index];
            if (sustain.Position < window.Begin || sustain.Position >= window.End || !sustain.Trigger.Evaluate(parameter, window.Stopping)) continue;
            if (keyOffs) { --keyOffs; continue; }
            if (!result || sustain.Position < result->Tick) result = LogicPoint{sustain.Position, index, {}, {}};
        }
    }
    for (std::size_t index = 0; index < timeline.Transitions.size(); ++index) {
        const auto& region = timeline.Transitions[index];
        if (window.LeadsOnly && !(region.Flags & 2)) continue;
        std::optional<std::uint32_t> position;
        if (region.Flags & 16) {
            const auto& destination = RequireMarker(timeline.Definition, region.Destination);
            const auto end = TimelineEnd(destination.Position, destination.Duration);
            if (window.Begin >= destination.Position && window.End < end) { rejected.erase(index); continue; }
            if (!region.Trigger.Evaluate(parameter, window.Stopping)) { rejected.erase(index); continue; }
            if (window.Begin < destination.Position) position = candidate(index, window.Begin, std::min(window.End, destination.Position), random);
            if (!position && window.End > end) position = candidate(index, std::max(window.Begin, end), window.End, random);
        } else {
            const bool point = region.Start == region.End;
            const bool include = window.IncludeStart || (region.Flags & 1);
            const bool overlaps = point ? (include ? window.Begin <= region.Start : window.Begin < region.Start) && region.Start <= window.End
                : window.Begin < region.End && region.Start < window.End;
            if (!overlaps || region.Destination == Bank::Guid{} || !region.Trigger.Evaluate(parameter, window.Stopping)) {
                rejected.erase(index);
                continue;
            }
            position = candidate(index, window.Begin, window.End, random);
        }
        if (!position) continue;
        if (!result || *position < result->Tick) result = LogicPoint{*position, {}, index, {}};
        else if (*position == result->Tick && result->Transition && !result->LeadTransition && (region.Flags & 2)) {
            const auto& selected = timeline.Transitions[*result->Transition];
            if (selected.Sequence && selected.Sequence->SourceOffset) result->LeadTransition = index;
        }
    }
    return result;
}
}
