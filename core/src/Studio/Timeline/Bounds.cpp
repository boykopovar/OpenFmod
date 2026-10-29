#include "OpenFmod/Studio/Timeline/Bounds.hpp"
#include <algorithm>
#include <limits>
namespace OpenFmod::Studio {
std::uint32_t TimelineEnd(std::uint32_t start, std::uint32_t duration) {
    const auto limit = std::numeric_limits<std::uint32_t>::max();
    return duration > limit - start ? limit : start + duration;
}
const NamedMarker& RequireMarker(const Timeline& timeline, const Bank::Guid& id) {
    const auto found = std::find_if(timeline.Markers.begin(), timeline.Markers.end(), [&](const auto& marker) { return marker.Id == id; });
    if (found == timeline.Markers.end()) throw FormatError("Timeline transition destination not found");
    return *found;
}
std::uint32_t TimelineDuration(const TimelineResource& timeline) {
    std::uint32_t result = 0;
    const auto& definition = timeline.Definition;
    for (const auto* schedule : {&definition.Triggered, &definition.TimeLocked}) {
        for (const auto& entry : *schedule) result = std::max(result, TimelineEnd(entry.Start, entry.Duration));
    }
    for (const auto& point : definition.SustainPoints) result = std::max(result, point.Position);
    for (const auto& transition : timeline.Transitions) {
        result = std::max({result, transition.Start, transition.End});
        if (transition.Sequence && transition.Sequence->Duration) {
            const auto& marker = RequireMarker(definition, transition.Destination);
            result = std::max(result, TimelineEnd(marker.Position, transition.Sequence->DestinationOffset));
        }
    }
    for (const auto& marker : definition.Markers) result = std::max(result, TimelineEnd(marker.Position, marker.Duration));
    return result;
}
}
